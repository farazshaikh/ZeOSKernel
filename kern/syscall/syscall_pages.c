/** @file     syscall_pages.c
 *  @brief    This file contains the system call handler for new_pages() , remove_pages()
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86/seg.h>
#include <malloc.h>
#include <exec2obj.h>
#include <elf_410.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include "bootdrvlib/timer_driver.h"
#include "bootdrvlib/keyb_driver.h"

#define PAGE_OFFSET_MASK 0xfff

#define PAGE_OFFSET( addr ) (addr)&PAGE_OFFSET_MASK


/** @function  syscall_newpages
 *  @brief     This function implements the new_pages system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */

KERN_RET_CODE syscall_newpages(void *user_param_packet) {
  int i;
  KERN_RET_CODE ret;
  void *base_addr;
  int len;
  ktask           *thisTask = (CURRENT_THREAD)->pTask;
  vm_range        vmrange;
  PDE             attributes;

  FN_ENTRY();

  attributes.PRESENT        = 1;
  attributes.RW             = 1;
  attributes.US             = 1;
  attributes.GLOBAL         = 0;

  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);
  base_addr  = (void *) (*(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0));


  if(CURRENT_THREAD->pTask->allocated_pages_mem + len > (unsigned long)ALLOC_MEM_QUOTA) {
    return KERN_NO_MEM;
  }


  // -- ensure that the base conditions are met -- //
  if( base_addr < (void *)USER_MEM_START )
    return KERN_PAGE_ERR;

  if( PAGE_OFFSET((unsigned long) base_addr))
    return KERN_PAGE_ERR;

  if( PAGE_OFFSET( len ))
    return KERN_PAGE_ERR;

  for(i=0;i<len;i+=PAGE_SIZE) {
    ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , base_addr+i , PAGE_SIZE );
    if( KERN_SUCCESS == ret )
      return KERN_PAGE_ERR;
  }

  // -- setup a vmrange to reflect the new pages to be added -- //
  memset( &vmrange, 0 , sizeof( vmrange ) );
  vmrange.start = ( unsigned long ) base_addr;
  vmrange.len = ( unsigned long ) len;


  // -- install the new pages using the vmm_install_range call -- //
  ret = vmm_install_range( &thisTask->vm , &vmrange );
  if( ret != KERN_SUCCESS )  {
    DUMP( "new pages install range failed %d" , ret );
    FN_LEAVE();
    return ret;
  }

  // -- initialize the new pages to be read-write at PDE and PTE -- //
  vmm_set_range_attr( &thisTask->vm, &vmrange , attributes);

  //- update quota -//
  CURRENT_THREAD->pTask->allocated_pages_mem += len;

  // -- Mark all PTEs in the range as not present -- //
  do {
    vmm_get_pte(&thisTask->vm,(uint32_t)base_addr)->PRESENT = 0;
    base_addr+=PAGE_SIZE;
    len -=  PAGE_SIZE;
  }while(len > 0);


  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  syscall_removepages
 *  @brief     This function implements the remove_pages system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */


KERN_RET_CODE syscall_removepages(void *user_param_packet) {
  KERN_RET_CODE ret = KERN_PAGE_ERR;
  void *base_addr;
  ktask           *thisTask = (CURRENT_THREAD)->pTask;
  vm_range        *vmrange;
  
  FN_ENTRY();
  base_addr  = (void *) GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  
  // -- ensure that the base conditions are met -- //
  if( base_addr < (void *)USER_MEM_START )
    return KERN_PAGE_ERR;
  
  if( PAGE_OFFSET((unsigned long) base_addr))
    return KERN_PAGE_ERR;
  
  // -- setup a vmrange to reflect the remove pages to be added -- //
  vmrange = vmm_get_range( &thisTask->vm , (char *)base_addr );
  if( vmrange == NULL )  {
    return KERN_ERROR_ADDRESS_NOT_PRESENT;
  }
  
  if(vmrange->start != (unsigned long)base_addr) { 
    return KERN_ERROR_ADDRESS_NOT_PRESENT;
  }
 
 // -- uninstall the pages using the vmm_uninstall_range call -- //
  CURRENT_THREAD->pTask->allocated_pages_mem -= vmrange->len;
  ret = vmm_uninstall_range( &thisTask->vm , vmrange );
  if( ret != KERN_SUCCESS )  {
    DUMP( "remove pages uninstall range failed %d" , ret );
    return ret;
  }

  FN_LEAVE();
  return KERN_SUCCESS;
}
