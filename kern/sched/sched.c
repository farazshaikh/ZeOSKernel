/** @file     sched.c 
 *  @brief    This file contains the scheduler related functions,
 *            and functions that setup the context switch between tasks
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86/seg.h>
#include <x86/cr.h>
#include <malloc.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "i386lib/i386systemregs.h"


scheduler kern_scheduler; 

/** @function  disable_preemption
 *  @brief     This function disables context switching (disable timer interrupts)
 *  @param     none
 *  @return    eflag status at the point of acquire-lock
 */

uint32_t disable_preemption(void) { 
  return spinlock_ifsave(&kern_scheduler.scheduler_lock);
}

/** @function  enable_preemption
 *  @brief     This function enables context switching (enable timer interrupts)
 *  @param     savedflags - flag status at the point of acquire-lock
 *  @return    void
 */

void enable_preemption(uint32_t savedflags) {
  return spinlock_ifrestore(&kern_scheduler.scheduler_lock , savedflags );
}

/** @function  sched_init
 *  @brief     This function initializes the scheduler
 *  @param     none
 *  @return    if all is well, this function never returns
 */

KERN_RET_CODE sched_init() { 
  KERN_RET_CODE ret; 
  FN_ENTRY();
  INIT_SCHEDULER(&kern_scheduler);

  /* task int */
  ret = task_init(INITIAL_BINARY);
  if( KERN_SUCCESS != ret ) { 
      DUMP("task_int() failed with ret=%d",ret);
      panic("task_init failed");
  }

  FN_LEAVE();
  return ret;
}

/** @function  _set_esp0
 *  @brief     This function sets the current stack pointer 
 *             to the top of kernel stack
 *  @param     none
 *  @return    void
 */

void _set_esp0() { 
  uint32_t kstack=0;
  kstack = ( uint32_t ) CURRENT_THREAD->context.kstack;
  set_esp0(kstack);
}

/** @function  context_switch
 *  @brief     This function sets up the context switch and switches
 *             the (uni)processor context from old_thread to new_thread
 *  @param     old_thread - pointer to the old/current thread
 *  @param     new_thread - pointer to the new/next thread
 *  @return    void
 */

static inline void context_switch(
				  kthread *old_thread,
				  kthread *new_thread)
{
  //- for consistency save restore format same as syscall_enter --//

  //- only instead of syscall code esp is pushed                --//
  //- in a way its perfect like every body is want to enter a   --//
  //- a syscall that saves thier esp stack                      --//
  __asm__ __volatile__ (
	   //- save all registers --//
	   SAVE_REGS
	   //- save esp           --//
	   "mov %%esp,%0;"

	   
	   //-- if old and new thread belong to different processes    --//
	   //-- reload PDBR (^-^)                                      --//
	   "mov %1,%%eax;" 
	   "cmp %2,%%eax;"
	   "je same_process;"
	   
	   //-- Relaod PDBR --//
	   "mov %3,%%eax;"
	   "mov %%eax,%%cr3;"

"same_process:"
	   // ki ki tik              //
	   // Kirk to USS-Enterprise //
	   // Ready to beam          // 
	   //           /\           //
	   //          /  \          //
	   //---------<Beam>---------//
	   //          \  /          //
	   //           \/           //
	   

	   //- load esp new       --  //
	   //- And don't look back 8) //
	   "mov %4,%%esp;"
 
	   //- restore all new    --//
	   RESTORE_REGS
	   :"=m" (old_thread->context.r_esp) 
	   :"m" (old_thread->pTask) , 
	    "m" (new_thread->pTask) , 
	    "m" (new_thread->pTask->vm.pde_base),
	    "m" (new_thread->context.r_esp)
	   :"%eax"  //-- Very Important directive --//
	   );

  _set_esp0();
  return;
}
 
/** @function  schedule
 *  @brief     This function manages scheduling between different threads
 *  @param     isCurrentRunnable - boolean representing 
 *                                 the runnable status of current thread
 *  @return    void
 */

void schedule(int isCurrentRunnable) { 
  FN_ENTRY();
  uint32_t savedflags;
  static   int inti;
  kthread *nextThread=NULL;
  kthread *thisThread = CURRENT_THREAD;

  // -- Assert running thread is not on the run queue -- //

  // -- LOCK SCHEDULER -- //
  savedflags = disable_preemption();

  // -- Get the next task to run -- //
  nextThread = Q_GET_FRONT(&kern_scheduler.run_queue);

  // -- don't run a thread that has a negetive run_flag -- //
  if(NULL != nextThread && nextThread->run_flag < 0) {
    scheduler_remove(nextThread);
    scheduler_add(nextThread);
    nextThread = NULL;
  }

  // -- if no other runnable threads, then schedule idle thread -- //
  inti++;
  if( NULL == nextThread /*|| inti % 2*/) {
    nextThread = get_idle_thread();
  }
   
  // -- Handles case where idle keeps calling the scheduler (No action) -- //
  // -- In this case next thread will be same as thisThread -- //
  if( nextThread != thisThread ) { 
    //- remove the next thread from the running queue -//
    if( nextThread != get_idle_thread() ) 
      scheduler_remove(nextThread);
    
    //- add this thread to runnable queue --//
    if( thisThread != get_idle_thread() && isCurrentRunnable ) 
      scheduler_add(CURRENT_THREAD);

    context_switch(thisThread,nextThread);
  }
  
  // -- UNLOCK SCHEDULER -- //
  enable_preemption(savedflags);
  FN_LEAVE();
} 


/** @function  scheduler_add
 *  @brief     This function adds the supplied thread to the tail of scheduler queue
 *  @param     thread - pointer to the thread to be added to the scheduler
 *  @return    void
 */

void scheduler_add(kthread *thread) { 
  uint32_t savedflags;
  FN_ENTRY();
  savedflags = disable_preemption();
  Q_INSERT_TAIL( &kern_scheduler.run_queue , thread , kthread_wait );
  enable_preemption(savedflags);
  FN_LEAVE();
}

/** @function  scheduler_remove
 *  @brief     This function removes the first thread in scheduler queue
 *             and returns the address of the thread
 *  @param     thread - pointer to the thread that was removed from the scheduler
 *  @return    void
 */

void scheduler_remove(kthread *thread) { 
  uint32_t savedflags;
  FN_ENTRY();
  savedflags = disable_preemption();
  Q_REMOVE( &kern_scheduler.run_queue , thread , kthread_wait );
  enable_preemption(savedflags);
  FN_LEAVE();
}

/** @function  scheduler_timer_callback
 *  @brief     This function calls scheduler and deschedules current thread
 *  @param     jiffies - clock ticks
 *  @return    void
 */
int timeslice;
#define TIME_QUANTUM 1
void scheduler_timer_callback(unsigned int jiffies) {
  FN_ENTRY();
  //DEBUG_PRINT("scheduler_timer_callback calling schedule");
  //- schedule isCurrentRunnable=1 is equivalent of yield -//

  //- Uncomment this if block to have variable time slice
  if(timeslice++ % TIME_QUANTUM == 0) {
    schedule(CURRENT_RUNNABLE);
  }
  FN_LEAVE();
}
