/** @file     syscall_fork.c
 *  @brief    This file contains the system call handler for fork()
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
#include <x86/cr.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include <sched.h>


/** @function  thread_setup_ret_from_fork
 *  @brief     This function sets up the initial thread stack 
 *             of the new task (iretframe, user context, context switch context)
 *  @param     thread - the initial_thread address of the forked task
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

void thread_setup_ret_from_fork(kthread      *thread) 
{
  i386_context *pParentU_context,parentU_context,switch_context;
  IRET_FRAME   *pParentIretFrame;
  


  switch_context.u.es = SEGSEL_KERNEL_DS;
  switch_context.u.ds = SEGSEL_KERNEL_DS;

  //- get stuff needed from the parent kernel stack -//
  pParentIretFrame = (IRET_FRAME *)
    ((char *)CURRENT_THREAD->context.kstack - sizeof(*pParentIretFrame));
  pParentU_context = (i386_context *)
    ((char *)pParentIretFrame - sizeof(*pParentU_context));
  memcpy(&parentU_context,pParentU_context,sizeof(*pParentU_context));
  
  //- set the return value for the child -//
  parentU_context.u.eax = 0;

  thread_setup_ret_from_syscall(thread,
				pParentIretFrame->esp,      // esp
			        pParentIretFrame->eip,      // eip
				0,                          // error code
				&parentU_context,
				&switch_context             // all start here
				);
  return;
}


/** @function  syscall_fork
 *  @brief     This function implements the fork system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    child task threadID to parent; 0 to child
 */

KERN_RET_CODE syscall_fork(void *user_param_packet) {
  KERN_RET_CODE ret;
  FN_ENTRY();

  ktask           *thisTask = (CURRENT_THREAD)->pTask;
  ktask           *newTask;
  kthread         *newThread;
  vm_range        *vmrange_ptr;
  PDE             attributes;


  attributes.PRESENT        = 1;
  attributes.RW             = 0;
  attributes.US             = 1;
  attributes.GLOBAL         = 0;



  // -- intialize the new task -- //
  task_fork_lock(thisTask);
  ret = vmm_init_task_vm( thisTask , &newTask );
  if( ret != KERN_SUCCESS )  {
    DUMP( "task Creation failed %d" , ret );
    task_fork_unlock(thisTask);    
    return ret;  
  }
  newThread = &newTask->initial_thread;

  // Install ranges in newTask //
  Q_FOREACH( vmrange_ptr , &(thisTask->vm).vm_ranges_head , vm_range_next ) {

  // -- Skip the element with start < USER_MEM_START as -- //
  // -- it is the kernel range which need not be copied -- //
  // -- onto the new task as it has its own copy -- //
    if( vmrange_ptr == &thisTask->vm.vm_range_kernel )
      continue;

    ret = vmm_install_range( &newTask->vm, vmrange_ptr );
    if( ret != KERN_SUCCESS )  {
      DUMP( "new task install range failed %d" , ret );
      task_fork_unlock(thisTask);    
      return ret;  
    }

    //- Make the child share the physical pages with parent --//
    ret = vmm_share_physical_range(&newTask->vm,
				   &thisTask->vm,
				   vmrange_ptr);
    if( ret != KERN_SUCCESS )  {
      DUMP( "cannot share pages between parent and child", ret );
      task_fork_unlock(thisTask);    
      return ret;  
    }
    
    //- Mark range Readonly in parent task -//
    vmm_set_range_attr( &thisTask->vm, vmrange_ptr , attributes);
    //- Mark range Readonly in child task -//
    vmm_set_range_attr( &newTask->vm, vmrange_ptr , attributes);
  }

  // Invalidate parents TLB //
  set_cr3((uint32_t)CURRENT_THREAD->pTask->vm.pde_base);
  thread_setup_ret_from_fork(newThread);
  scheduler_add( newThread );
  FN_LEAVE();

  task_fork_unlock(thisTask);    
  return (KERN_RET_CODE) newThread;
}

