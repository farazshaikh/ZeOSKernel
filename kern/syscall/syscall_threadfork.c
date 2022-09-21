/** @file     threadfork.c
 *  @brief    This file contains the system call handler for threadfork()
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

void BFN() {}

/** @function  syscall_threadfork
 *  @brief     This function implements the thread_fork system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    child tid to parent; 0 to child
 */

KERN_RET_CODE syscall_threadfork(void *user_param_packet) {

  kthread         *thisThread = (CURRENT_THREAD);
  ktask           *thisTask = thisThread->pTask;
  kthread         *newThread;
  char *threadmem;

  FN_ENTRY();

  // -- Setup the thread kernel stack -- //
  //
  //   Thread Stack Image
  //  ====================
  //   __________________
  //  |                  |GUARD
  //  |                  |GUARD
  //  |                  |<- kstack
  //  |                  |IRET FRAME
  //  |                  |IRET FRAME
  //  |                  |USERMODE SWITCH CONTEXT
  //  |                  |USERMODE SWITCH CONTEXT
  //  |                  |CLOBBER REG
  //  |                  |CLOBBER REG
  //  |                  |CONTEXT SWITCH CONTEXT
  //  |                  |CONTEXT SWITCH CONTEXT
  //  |                  |<- r_esp
  //  .                  .
  //  .                  .
  //  |                  |
  //  |                  |<-thread struct end
  //  |                  |
  //  |                  |
  //  |__________________|<-thread struct start
  //
  // --                              -- //

  threadmem = smemalign( PAGE_SIZE * KTHREAD_KSTACK_PAGES, PAGE_SIZE * KTHREAD_KSTACK_PAGES );
  if( NULL == threadmem )
    return KERN_NO_MEM;
  memset( threadmem , 0 , PAGE_SIZE * KTHREAD_KSTACK_PAGES );

  newThread = (kthread *) threadmem;
  newThread->pTask = thisTask;
  newThread->context.kstack = (STACK_ELT *)(threadmem + (PAGE_SIZE * KTHREAD_KSTACK_PAGES));
  newThread->context.kstack--; // -- GUARD
  newThread->context.kstack--; // -- GUARD
  newThread->context.kstack--; // -- GUARD
  newThread->context.kstack--; // -- GUARD

  newThread->context.r_esp = newThread->context.kstack;

  task_fork_lock(thisTask);

  // -- Add the forked thread as the next thread of the parent thread -- //
  // -- Useful when reaping dead parent -- //

  BFN();

  Q_INIT_ELEM( newThread , kthread_next);
  Q_INSERT_FRONT( &thisTask->ktask_threads_head,
		  newThread,
		  kthread_next);

  thread_setup_ret_from_fork(newThread);
  scheduler_add( newThread );

  task_fork_unlock(thisTask);

  FN_LEAVE();
  return (KERN_RET_CODE) newThread;
}
