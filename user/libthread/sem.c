/** @file sem.c
 *  @brief This file defines functions related to semaphore type
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */


#include <sem.h>
#include <syscall.h> /*for cas2i_runflag*/
#include "thr_internals.h"


/** @brief This function initializes the semaphore pointed to by sem.
 *         The effects of using a semaphore before it has been initialized,
 *         or of initializing it when it is already initialized and in use,
 *         are undefined.
 *
 *  @param sem - The pointer to semaphore
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */


int sem_init( sem_t *sem, int count ) {
  SEM_INIT(sem,count);
  return ETHREAD_SUCCESS;
}


/** @brief This function destroys the semaphore pointed to by sem.
 *         The effects of destroying a semaphore before it has been initialized is undefined.
 *
 *
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR if the semaphore is active
 *          asserts on failure
 */


int sem_destroy( sem_t *sem ) {
  lock_sem_mutex( sem );
  if(!DLIST_EMPTY(&sem->semWaitControlBlock.waiters_anchor)) {
    unlock_sem_mutex( sem );
    return ETHREAD_BUSY;
  }

  SEM_INIT(sem,0);
  return ETHREAD_SUCCESS;

}


/** @brief This function allows a thread to either enter the critical section
 *         if the number of threads in the section is less than count
 *         or if the number of threads in critical section = count
 *         then the thread is suspended and its state is set to WAITING
 *         until woken up by a condition variable
 *
 *  @param sem The pointer to semaphore
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR and asserts on failure
 */


int sem_wait( sem_t *sem ) {
  PTHREAD_CNTRL_BLCK pThis_thread;

  //-- Protect Against ones' pending fork --//
  lockTaskControlBlock();
  pThis_thread = getThreadControlBlock(THIS_THREAD);
  assert( NULL != pThis_thread );


  lock_sem_mutex( sem );
  sem->count--;
  //-- Sem available --//
  if(sem->count >= 0) {
    unlockTaskControlBlock();
    unlock_sem_mutex( sem );
    return ETHREAD_SUCCESS;
  }

  //-- Sem not available --//
  assert( pThis_thread->state == THREAD_STATE_RUNNABLE );
  sleep_on_no_schedule( &sem->semWaitControlBlock );

  //-- sleep and release the mutex --//
  //-- This specifically causes bouncing back and forth -//
  pThis_thread->state = THREAD_STATE_WAITING;
  unlockTaskControlBlock();
  cas2i_runflag(gettid(),
		(int *)&sem->semWaitControlBlock.waiters_list_mutex,
		RUN_STATE_RUNNING,
		RUN_STATE_STOPPED,
		RUN_STATE_INVALID,
		RUN_STATE_INVALID);
  pThis_thread->state = THREAD_STATE_RUNNABLE;

  return ETHREAD_SUCCESS;
}


/** @brief This function wakes up a thread waiting on the
 *         semaphore pointed to by sem, if one exists.
 *
 *  @param sem - The pointer to semaphore
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR and asserts on failure
 */


int sem_signal( sem_t *sem ) {
  int ret;
  PTHREAD_CNTRL_BLCK pThis_thread;
  pThis_thread = getThreadControlBlock(THIS_THREAD);
  assert( NULL != pThis_thread );

  lock_sem_mutex( sem );
  ++sem->count;
  if(sem->count <= 0) {
    ret = wakeup_first_waiter( &sem->semWaitControlBlock ,NULL );
    assert(ret == ETHREAD_SUCCESS);
  }
  unlock_sem_mutex( sem );
  return ETHREAD_SUCCESS;
}



