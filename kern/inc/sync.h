/** @file     sync.h
 *  @brief    This file defines the process synchronization primitives
 *            (spinlocks and semaphores) and their initialization functions
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _SYNC_H
#define _SYNC_H

#include <kern_common.h>
#include <x86/page.h>
#include <types.h>

// -- Controls code for SMP spinlocks -- //
// -- Uncomment the following like for enabling SMP spinlock implementation -- //
//  #define SMP 1

typedef struct _spinlock {
#ifdef SMP
  volatile unsigned long lock;
#endif
}spinlock;

#define SPINLOCK_INIT( plock ) do {					\
    memset( (plock),0,sizeof(*(plock)) );				\
  }while(0)

#define SPINLOCK_DESTROY( plock ) do {					\
    if( (plock)->lock )							\
      panic( "KERNEL PANIC: cannot destroy spinlock %p",(plock) );	\
    memset((plock),0,sizeof(*(plock)));					\
  }while(0)

KERN_RET_CODE spinlock_lock ( spinlock *pspinlock );
KERN_RET_CODE spinlock_unlock ( spinlock *pspinlock );

uint32_t spinlock_ifsave( spinlock *pspinlock );
void spinlock_ifrestore( spinlock *pspinlock, uint32_t savedflags );

Q_NEW_HEAD( sem_wait_head , kthread );

typedef struct _semaphore {
  spinlock lock;
  volatile int count;
  sem_wait_head sem_kthread_head;
}semaphore;

#define SEMAPHORE_INIT( psem , val ) do {				\
    memset( (psem),0,sizeof(*(psem)) );					\
    (psem)->count = (val);						\
    SPINLOCK_INIT( &(psem)->lock );					\
    Q_INIT_HEAD( &(psem)->sem_kthread_head );				\
  }while(0)

#define SEMAPHORE_DESTROY( psem ) do {					\
    if( (psem)->count )							\
      panic("KERNEL PANIC: cannot destroy semaphore %p",(psem));	\
    memset( (psem),0,sizeof(*(psem)) );					\
    SPINLOCK_INIT( &(psem)->lock );					\
    Q_INIT_HEAD( &(psem)->sem_kthread_head );				\
  }while(0)

KERN_RET_CODE sem_wait   ( semaphore *psemaphore );
KERN_RET_CODE sem_signal ( semaphore *psemaphore );
int           sem_waiters( semaphore *psemaphore );
#endif // _SYNC_H
