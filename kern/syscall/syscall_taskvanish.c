/** @file     syscall_taskvanish.c
 *  @brief    This file contains the system call handler for task_vanish()
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

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"


/** @function  syscall_taskvanish
 *  @brief     This function implements the task_vanish system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */

KERN_RET_CODE syscall_taskvanish(void *user_param_packet) { 

  ktask *thisTask = (CURRENT_THREAD)->pTask;
  kthread *thread;

  FN_ENTRY();
  DUMP("syscall task_vanish on task %p",thisTask);

  //-- Temporary hack                            --//
  //-- Remove your self from the scheduler queue --//
  task_fork_lock(thisTask);

  Q_FOREACH( thread , &thisTask->ktask_threads_head , kthread_next ) {

      Q_REMOVE( &thisTask->ktask_threads_head , thread , kthread_next );
      scheduler_remove(thread);
      if(CURRENT_THREAD->pTask->ktask_threads_head.nr_elements == 0) {
	CURRENT_THREAD->pTask->state = TASK_STATUS_ZOMIE;
	//- we cannot be scheduled anymore now -//
	sem_signal(&CURRENT_THREAD->pTask->parentTask->vultures);
      }

  }
  
  task_fork_unlock(thisTask);
  
  //- you may or may not come back here-//
  schedule(0);

  //- you definitely will not run this code -//
  assert(0);
  return KERN_SUCCESS;
}
