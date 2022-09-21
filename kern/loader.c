/** @file     loader.c
 *  @brief    This file contains the loader function definitions
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>
#include <x86/cr.h>

//-- kernel include --//
#include <kern_common.h>
#include <loader_internal.h>
#include <elf_410.h>
#include <stddef.h>


/** @function  getbytes
 *  @brief     This function copies data from a file into a buffer.
 *
 *  @param     filename - the name of the file to copy data from
 *  @param     offset - the location in the file to begin copying from
 *  @param     size - the number of bytes to be copied
 *  @param     buf - the buffer to copy the data into
 *
 *  @return    returns the number of bytes copied on succes; -1 on failure
 */

int getbytes( const char *filename, int offset, int size, char *buf ) {
  int index;
  int found = 0;
  
  FN_ENTRY();
  index = 0;

  // -- find the file -- //
  while(index < exec2obj_userapp_count) {
    // match //
    if(!strcmp(filename, exec2obj_userapp_TOC[index].execname)) {
      found = 1;
      break;
    }
    index++;
  }

  if(!found)  {
    DUMP("Executalble not found %s!!",filename);
    return -1;
  }

  // -- does this file have any data -- //
  if((char *)exec2obj_userapp_TOC[index].execbytes == NULL) {
    DUMP("Executalble cannot be read %s!!",filename);
    return -1;
  }

  // -- copy stuff over -- //
  memcpy(buf,
	 (char *)exec2obj_userapp_TOC[index].execbytes + offset,
	 size);

  FN_LEAVE();
  return size;
}

/** @function  loader_install_ranges
 *  @brief     This function installs the different ELF sections of the file
 *             into the VM of the supplied task
 *
 *  @param     vm - pointer to the VM manager of a task
 *  @param     fname - name of the file in the ramdisk that must be loaded
 *
 *  @return    KERN_SUCCESS on success 
 *             Else an appropriate error code on failure
 */

KERN_RET_CODE loader_install_ranges( task_vm *vm , char *fname )  {
  KERN_RET_CODE ret;
  simple_elf_t se_hdr;
  vm_range     vm_range;
  PDE          attr;

  if( ELF_SUCCESS != elf_check_header( fname ) )  {
    DUMP(" elf_check_header failed %s", fname);
    return KERN_NOT_AN_ELF;
  }
  
  //-- read the header --//
  if( ELF_SUCCESS != elf_load_helper(&se_hdr,fname) ) {
    DUMP(" elf_load_helper failed %s", fname);
    return KERN_NOT_AN_ELF;
  }

  //-- Install all the user mode ranges in new vm --//
  //-- .text
  vm_range.start  = se_hdr.e_txtstart;
  vm_range.len    = se_hdr.e_txtlen;
  ret = vmm_install_range(vm,&vm_range);
  if(KERN_SUCCESS != ret) {
    DUMP("failed to install text range");
    goto err;
  }
  vm->vm_text_start = vm_range.start; 
  vm->vm_text_len   = vm_range.len;
  //--  set attributes --//
  memset(&attr,0,sizeof(attr));
  attr.PRESENT      = 1;
  attr.RW           = 1;
  attr.US           = 1;
  attr.GLOBAL       = 0;
  vmm_set_range_attr( vm , &vm_range , attr );

  //-- .rodata
  vm_range.start  = se_hdr.e_rodatstart;
  vm_range.len    = se_hdr.e_rodatlen;
  ret = vmm_install_range(vm,&vm_range);
  if(KERN_SUCCESS != ret) {
    DUMP("failed to install readonly data range");
    goto err;
  }
  vm->vm_rdata_start = vm_range.start; 
  vm->vm_rdata_len   = vm_range.len;
  //--  set attributes --//
  memset(&attr,0,sizeof(attr));
  attr.PRESENT      = 1;
  attr.RW           = 1;
  attr.US           = 1;
  attr.GLOBAL       = 0;
  vmm_set_range_attr( vm , &vm_range , attr );


  //-- .heap
  //-- .bss
  vm_range.start  = se_hdr.e_datstart;
  vm_range.len    = se_hdr.e_datlen + se_hdr.e_bsslen;
  ret = vmm_install_range(vm,&vm_range);
  if(KERN_SUCCESS != ret) {
    DUMP("failed to install data range");
    goto err;
  }
  vm->vm_data_start = vm_range.start; 
  vm->vm_data_len   = vm_range.len;
  //--  set attributes --//
  memset(&attr,0,sizeof(attr));
  attr.PRESENT      = 1;
  attr.RW           = 1;
  attr.US           = 1;
  attr.GLOBAL       = 0;
  vmm_set_range_attr( vm , &vm_range , attr );



  //-- .stack
  vm_range.len    = KTHREAD_USTACK_PAGES * PAGE_SIZE;
  vm_range.start  = 0xffffa000;
  ret = vmm_install_range(vm,&vm_range);
  if(KERN_SUCCESS != ret) {
    DUMP("failed to install text range");
    goto err;
  }
  vm->vm_stack_start = vm_range.start; 
  vm->vm_stack_len   = vm_range.len;
  //--  set attributes --//
  memset(&attr,0,sizeof(attr));
  attr.PRESENT      = 1;
  attr.RW           = 1;
  attr.US           = 1;
  attr.GLOBAL       = 0;
  vmm_set_range_attr( vm , &vm_range , attr );


  //-- back all the ranges with physical pages --//
  ret = vmm_back_all_user_ranges( vm );
  if( KERN_SUCCESS != ret ) {
    goto err;
  }


  return KERN_SUCCESS;
 err:
  vmm_unback_all_user_ranges( vm );
  vmm_free_user_ptes( vm );
  vmm_free_all_vma( vm );
  return ret;
}

/** @function  load_elf
 *  @brief     This function spawns a new process and loads the elf file sections
 *             into the new task's VM, and then unloads the current task
 *             and vanishes the current task. 
 *  // -- probably redundant design decision   -- //
 *  // -- tweaked to do the right thing, instead of having to rewrite -- // 
 *
 *  @param     task - pointer to task
 *  @param     fname - name of the file in the ramdisk that must be loaded
 *  @param     start_address - start address of the binary loaded in text section
 *  @param     u_stack - userland stack start address 
 *
 *  @return    KERN_SUCCESS on success 
 *             Else an appropriate error code on failure
 */

KERN_RET_CODE load_elf(ktask         *task , 
		       char          *fname ,
		       unsigned long *start_address,
		       unsigned long *u_stack
		       )
{
  KERN_RET_CODE ret;
  ktask         *vm_task;
  kthread       *this_thread = CURRENT_THREAD;
  PDE           attr;
  simple_elf_t  se_hdr;
  vm_range      vm_range;

  this_thread = this_thread;


  if( ELF_SUCCESS != elf_check_header( fname ) )  {
    DUMP(" elf_check_header failed %s", fname);
    return KERN_NOT_AN_ELF;
  }

  ret = vmm_init_task_vm( NULL , &vm_task );
  if( ret != KERN_SUCCESS )  {
    DUMP( "task Creation failed %d" , ret );
    return ret;  
  }

  //-- Install all the ranges --//
  ret = loader_install_ranges(&vm_task->vm,fname);
  if( KERN_SUCCESS != ret ) {
    goto err;
  }

  //-- free up the old user space VM mapping of old address space --//
  ret = vmm_unback_all_user_ranges(&CURRENT_THREAD->pTask->vm);
  if( ret != KERN_SUCCESS )  {
    DUMP( "Memory leak Cannot Unback pages from old address space %d" , ret );
  }
  ret = vmm_free_user_ptes(&CURRENT_THREAD->pTask->vm);
  if( ret != KERN_SUCCESS )  {
    DUMP( "Memory leak Cannot free pages from old address space %d" , ret );
  }
  vmm_free_all_vma( &CURRENT_THREAD->pTask->vm );


  //-- claim up user mode mapping from new vm --//
  ret = vmm_copy_user_ptes(&CURRENT_THREAD->pTask->vm,&vm_task->vm);
  if( ret != KERN_SUCCESS )  {
    DUMP( "Failed copying address space address space %d" , ret );
    assert(0); //-- kill process --//
  }

  //- invalidate tlb as we just updated page mapping --//
  set_cr3((uint32_t)CURRENT_THREAD->pTask->vm.pde_base);

  //-- copy all ranges into new VM                --//
  //-- .text
  //-- .rodata
  //-- .heap
  //-- .bss
  //-- .stack
  //-- read the header --//
  if( ELF_SUCCESS != elf_load_helper(&se_hdr,fname) ) {
    DUMP(" Strangely elf_load_helper failed %s", fname);
    assert(0);
    return KERN_NOT_AN_ELF;
  }

  
  ret = getbytes(fname,
		 se_hdr.e_txtoff,
		 se_hdr.e_txtlen,
		 (char *)se_hdr.e_txtstart);
  if(ret != se_hdr.e_txtlen) {
    DUMP(" getbytes text failed %s", fname);
    assert(0);
    ret = KERN_NOT_AN_ELF;
    goto err;
  }

  ret = getbytes(fname,
		 se_hdr.e_datoff,
		 se_hdr.e_datlen,
		 (char *)se_hdr.e_datstart);
  if(ret != se_hdr.e_datlen) {
    DUMP(" getbytes data failed %s", fname);
    assert(0);
    ret = KERN_NOT_AN_ELF;
    goto err;
  }
  memset((char *)se_hdr.e_datstart + se_hdr.e_datlen,
	 0,
	 se_hdr.e_bsslen);


  ret = getbytes(fname,
		 se_hdr.e_rodatoff,
		 se_hdr.e_rodatlen,
		 (char *)se_hdr.e_rodatstart);
  if(ret != se_hdr.e_rodatlen) {
    DUMP(" getbytes rodata failed %s", fname);
    assert(0);
    ret = KERN_NOT_AN_ELF;
    goto err;
  }


  // Mark regions readonly //
  memset(&attr,0,sizeof(attr));
  attr.PRESENT      = 1;
  attr.RW           = 0;
  attr.US           = 1;
  attr.GLOBAL       = 0;
  //. text
  vm_range.start  = se_hdr.e_txtstart;
  vm_range.len    = se_hdr.e_txtlen;
  vmm_set_range_attr( &CURRENT_THREAD->pTask->vm , &vm_range , attr );
  //. rodata  
  vm_range.start  = se_hdr.e_rodatstart;
  vm_range.len    = se_hdr.e_rodatlen;
  vmm_set_range_attr( &CURRENT_THREAD->pTask->vm , &vm_range , attr );

  //- copy over all ranges from one task to other -//
  ret = vmm_copy_vmranges_struct(&CURRENT_THREAD->pTask->vm,
			   &vm_task->vm);
  if(ret != KERN_SUCCESS) {
    DUMP(" PRETTY LOW ON MEMORY WILL MESS UP SOON !!");
    goto err;
  }
  
  
  //-- free all VMA's 
  //-- and kernel stack we have claimed its PTE's and page backings -//
  vmm_free_all_vma(&vm_task->vm);
  sfree(vm_task->vm.taskmem,
	vm_task->vm.totalTaskAllocation);
  
  *start_address = se_hdr.e_entry;
  *u_stack       = vm_task->vm.vm_stack_start + vm_task->vm.vm_stack_len;
  return KERN_SUCCESS;
  
 err: 
  vmm_unback_all_user_ranges( &vm_task->vm );
  vmm_free_user_ptes( &vm_task->vm );

  vmm_free_all_vma( &vm_task->vm );
  sfree(vm_task->vm.taskmem,
	vm_task->vm.totalTaskAllocation);
  return ret;
}


