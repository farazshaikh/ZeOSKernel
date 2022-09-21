/** @file     syscall_vanish.c
 *  @brief    This file contains the system call handler for vanish()
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


/** @function  syscall_vanish
 *  @brief     This function implements the vanish system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */

KERN_RET_CODE syscall_vanish(void *user_param_packet) { 

  ktask *thisTask = (CURRENT_THREAD)->pTask;
  kthread *thread;

  FN_ENTRY();
  DUMP("syscall vanish on thread %p",CURRENT_THREAD);

  //-- Temporary hack                            --//
  //-- Remove your self from the scheduler queue --//
  task_fork_lock(thisTask);
  
  (CURRENT_THREAD)->run_flag = -1;
  scheduler_remove(CURRENT_THREAD);

  Q_FOREACH( thread , &thisTask->ktask_threads_head , kthread_next ) {
    if(thread == (CURRENT_THREAD)) {
      Q_REMOVE( &thisTask->ktask_threads_head , thread , kthread_next );
      if(thisTask->ktask_threads_head.nr_elements == 0) {
	thisTask->state = TASK_STATUS_ZOMIE;
	//- we cannot be scheduled anymore now -//
	sem_signal(&thisTask->parentTask->vultures);
      }
      break;
    }
  }
  
  task_fork_unlock(thisTask);
  
  //- you may or may not come back here-//
  schedule(0);

  //- you definitely will not run this code -//
  assert(0);
  return KERN_SUCCESS;
}
