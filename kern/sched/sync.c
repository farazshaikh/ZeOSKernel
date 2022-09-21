/** @file     sync.c
 *  @brief    This file contains the definition of synchronization primitives
 *            and definitions of function calls that manipulate them
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
#include <cr.h>
#include <eflags.h>

#include <kern_common.h>
#include <sched.h>
#include <variable_queue.h>
#include <sync.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "i386lib/i386systemregs.h"
#include "i386lib/i386saverestore.h"

#define ASM_SPINLOCK do {				\
    __asm__ (						\
	     "movl $1, %%eax;"				\
	     "movl %0, %%edx;"				\
	     "1:"					\
	     "xchg %%eax,(%%edx);"	                \
	     "cmpl $0,%%eax;"				\
	     "je 3f;"					\
	     "2:"					\
	     "cmpl $1,(%%edx);"				\
	     "je 2b;"					\
	     "jmp 1b;"					\
	     "3:"					\
	     "nop;"					\
	     :						\
	     :"m" (&pspinlock->lock)			\
	     :"%edx","%eax");				\
  }while(0)

#define ASM_SPINUNLOCK do {					\
    __asm__ (							\
	     "movl $0, %%eax;"					\
	     "movl %0, %%edx;"					\
	     "xchg %%eax,(%%edx);"				\
	     :							\
	     :"m" (&pspinlock->lock)				\
	     :"%edx","%eax");				\
  }while(0)

/** @function  spinlock_lock
 *  @brief     This function is used to lock the supplied spinlock
 *             On a uniprocessor, this is equal to a "CLI" instruction.
 *  @param     pspinlock - pointer to the spinlock that must be locked
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE spinlock_lock ( spinlock *pspinlock ) {
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  disable_interrupts();

#ifdef SMP

  ASM_SPINLOCK;

  assert( 1 == pspinlock->lock );

#endif

  FN_LEAVE();
  return ret;
}

/** @function  spinlock_unlock
 *  @brief     This function is used to unlock the supplied spinlock
 *             On a uniprocessor, this is equal to a "STI" instruction.
 *  @param     pspinlock - pointer to the spinlock that must be unlocked
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE spinlock_unlock ( spinlock *pspinlock ){
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

#ifdef SMP

  ASM_SPINUNLOCK;

  assert( 0 == pspinlock->lock );

#endif

  enable_interrupts();

  FN_LEAVE();
  return ret;
}

/** @function  spinlock_ifsave
 *  @brief     This function is used to lock the supplied spinlock
 *             On a uniprocessor, this is equal to a "CLI" instruction.
 *  @param     pspinlock - pointer to the spinlock that must be locked
 *  @return    the eflags status at the point of spinlock_lock call
 */

uint32_t spinlock_ifsave( spinlock *pspinlock ) {
  uint32_t saved_flags = get_eflags();
  disable_interrupts();
  FN_ENTRY();

#ifdef SMP
#error "If you are building SMP spinlocks, remove this line"
  ASM_SPINLOCK;

  assert( 1 == pspinlock->lock );

#endif

  FN_LEAVE();

  return saved_flags;
}

/** @function  spinlock_ifrestore
 *  @brief     This function is used to unlock the supplied spinlock
 *             On a uniprocessor, this is equal to a "STI" instruction.
 *  @param     pspinlock  - pointer to the spinlock that must be unlocked
 *  @param     savedflags - status of eflags during spinlock_ifsave
 *  @return    void
 */

void spinlock_ifrestore( spinlock *pspinlock, uint32_t savedflags ) {

 FN_ENTRY();

#ifdef SMP

  ASM_SPINUNLOCK;

  assert( 0 == pspinlock->lock );

#endif

  set_eflags(savedflags);

  FN_LEAVE();

}

/** @function  sem_wait
 *  @brief     This function is used by a thread to wait on a semaphore
 *             It decrements the sem count and sleeps if count is less than 0
 *  @param     psemaphore  - pointer to the semaphore that must be waited on
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE sem_wait ( semaphore *psemaphore ){
  KERN_RET_CODE ret;
  kthread       *thisThread = CURRENT_THREAD;
  uint32_t      eflags;

  FN_ENTRY();

  // --- lock the spinlock before updating thread lists --- //
  eflags = spinlock_ifsave( &psemaphore->lock );

  psemaphore->count--;

  if( psemaphore->count >= 0 ) {
    ret = spinlock_unlock( &psemaphore->lock );
    assert( ret == KERN_SUCCESS );
    return KERN_SUCCESS;
  }

  // -- The following two calls can run in any order  -- //
  // -- as there will be no context switch in between -- //

  // -- Running thread is already removed from the runnable queue --//
  // -- add the current thread to the semaphore wait queue        --//
  Q_INSERT_TAIL( &psemaphore->sem_kthread_head , thisThread , kthread_wait );

  // --- unlock the spinlock after updating thread lists --- //
  spinlock_ifrestore( &psemaphore->lock , eflags );

  //-- schedule notifying that current is not eligible for running --//
  //-- until sem signals                                           --//
  schedule(0);

  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  sem_signal
 *  @brief     This function is used by a thread to signal a semaphore
 *             It increments the sem count, and if count is less than or 0
 *             it wakes up the first thread from the sem wait queue and schedules it
 *  @param     psemaphore  - pointer to the semaphore that must be signalled
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE sem_signal ( semaphore *psemaphore ){
  KERN_RET_CODE ret;
  kthread       *wakeupThread = NULL;
  uint32_t      eflags;

  FN_ENTRY();

  // --- lock the spinlock before updating thread lists --- //
  eflags = spinlock_ifsave( &psemaphore->lock );

  psemaphore->count++;

  if( psemaphore->count > 0 ) {
    ret = spinlock_unlock( &psemaphore->lock );
    assert( ret == KERN_SUCCESS );
    return KERN_SUCCESS;
  }

  // -- extract the first sleeping thread in the semaphore wait queue -- //
  wakeupThread = Q_GET_FRONT( &psemaphore->sem_kthread_head );



  // -- call that adds wakeupThread to kern_scheduler runqueue -- //
  if( wakeupThread ) {
    Q_REMOVE( &psemaphore->sem_kthread_head , wakeupThread , kthread_wait );
    scheduler_add( wakeupThread );
  }

  // --- unlock the spinlock after updating thread lists --- //
  spinlock_ifrestore( &psemaphore->lock , eflags );

  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function  sem_waiters
 *  @brief     This function is used to extract the number of threads
 *             that are sleeping on a semaphore
 *  @param     psemaphore  - pointer to the semaphore
 *  @return    integer number of threads waiting on the considered semaphore
 */

int sem_waiters( semaphore *psemaphore ) {
  return psemaphore->sem_kthread_head.nr_elements;
}
