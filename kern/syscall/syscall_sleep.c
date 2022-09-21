/** @file     syscall_sleep.c
 *  @brief    This file contains the system call handler for sleep()
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


Q_NEW_HEAD( slackers_kthread_head , kthread );
slackers_kthread_head kern_slackers_kthread_head;

/** @function  syscall_sleep
 *  @brief     This function implements the sleep system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on successful completion
 */

KERN_RET_CODE syscall_sleep(void *user_param_packet) {
  int ticks; 
  FN_ENTRY();

  // -- save the argument into ticks -- //
  ticks = (int)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  CURRENT_THREAD->sleepticks = ticks;

  // -- place the thread into the slackers queue -- //
  Q_INSERT_TAIL( &kern_slackers_kthread_head , CURRENT_THREAD , kthread_wait);

  // -- deschedule self -- //
  schedule( CURRENT_NOT_RUNNABLE );

  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  sleep_init
 *  @brief     This function initializes the sleep queue
 *  @param     none
 *  @return    KERN_SUCCESS on successful completion
 */

KERN_RET_CODE sleep_init() { 
  FN_ENTRY();
  // -- initialize the slackers queue -- //
  Q_INIT_HEAD(&kern_slackers_kthread_head);
  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function  sleep_bottom_half
 *  @brief     This function decrements the sleep ticks in all sleeping threads
 *             for every tick, and wakes up threads whose ticks expire
 *  @param     none
 *  @return    KERN_SUCCESS on successful completion
 */

KERN_RET_CODE sleep_bottom_half() { 
  kthread *threadTrav,*threadSaveTrav;
  FN_ENTRY();
  Q_FOREACH_DEL_SAFE(threadTrav, &kern_slackers_kthread_head,kthread_wait,threadSaveTrav)  {
    //- is any thread ready to scheduled -//
    threadTrav->sleepticks--;
    if(threadTrav->sleepticks <= 0) {
      Q_REMOVE(&kern_slackers_kthread_head , threadTrav , kthread_wait );
      scheduler_add(threadTrav);
    }
  }
  FN_LEAVE();
  return KERN_SUCCESS;
}
