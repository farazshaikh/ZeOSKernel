/** @file cond.c
 *  @brief This file defines functions related to condition variable type
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#include <cond.h>
#include <syscall.h> /*for cas2i_runflag*/
#include "thr_internals.h"


/** @brief This function initializes the condition variable pointed to by cv.
 *         The effects of using a condition var before it has been initialized,
 *         or of initializing it when it is already initialized and in use,
 *         are undefined.
 *
 *  @param cv The pointer to condition variable
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */


int cond_init( cond_t *cv ) {
  INIT_COND_VAR( cv );
  return ETHREAD_SUCCESS;
}


/** @brief This function destroys the condition variable pointed to by cv.
 *         The effects of using a condition variable before it has been initialized,
 *         or of initializing it when it is already initialized and in use,
 *         are undefined.
 *
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR asserts on failure
 */


int cond_destroy( cond_t *cv ) {
  lock_wait_control_block(&cv->condWaitControl);

  if(!DLIST_EMPTY(&cv->condWaitControl.waiters_anchor)) {
    unlock_wait_control_block(&cv->condWaitControl);
    return ETHREAD_ERR;
  }

  //- Destroy the stuff with the lock held --//
  INIT_COND_VAR( cv );
  return ETHREAD_SUCCESS;
}


/** @brief This function allows a thread to wait for a condition
 *         and release the associated mutex that it needs to hold
 *         to check that condition. The calling thread blocks,
 *         waiting to be signaled. The blocked thread may be awakened
 *         by a cond signal() or a cond broadcast().
 *
 *  @param cv The pointer to condition variable
 *  @param mp The pointer to the mutex to condition wait on
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR asserts on failure
 */

int cond_wait( cond_t *cv, mutex_t *mp ) {
  PTHREAD_CNTRL_BLCK pThis_thread;


  //-- Protects around ones' own Fork --//

  //-- Here we have a special case --//
  //-- The thread library is a world and its states is --//
  //-- protected by the world lock                     --//
  //-- The thread library itself uses cond-vars to     --//
  //-- to implement joins and exits                    --//
  //   a)
  //-- Thread JOIN and EXIT and CREATE are protect by  --//
  //-- Thread world lock.                              --//
  //-- b) condvar also use the thread world lock for   --//
  //-- protecting a threads referece to itself before  --//
  //-- its fork                                        --//

  //-- A and B both occur in the case where a thread   --//
  //-- calls operations in a i.e JOIN which involves   --//
  //-- locking the Thread World lock and calling condwait--//
  //-- and trying to release it atomically             --//
  //-- In this case the CONDWAIT must not re-acquire Thread --//
  //-- World lock                                      --//

  if(!isMutexThreadWorldLock(mp))
    lockTaskControlBlock();



  pThis_thread = getThreadControlBlock(THIS_THREAD);



  if( NULL == pThis_thread )
    SIM_break();

  assert( NULL != pThis_thread );


  lock_wait_control_block(&cv->condWaitControl);
  sleep_on_unprotected(&cv->condWaitControl);

  pThis_thread->state = THREAD_STATE_WAITING;
  mutex_unlock( mp );

  if(!isMutexThreadWorldLock(mp)) //- Else it would double unlock in case mp==TWM
    unlockTaskControlBlock();

  cas2i_runflag(gettid(),
		(int *)&cv->condWaitControl.waiters_list_mutex.is_locked,
		RUN_STATE_RUNNING,
		RUN_STATE_STOPPED,
		RUN_STATE_INVALID,
		RUN_STATE_INVALID);
  pThis_thread->state = THREAD_STATE_RUNNABLE;
  mutex_lock(mp);
  return ETHREAD_SUCCESS;
}


/** @brief This function wakes up a thread waiting on the
 *         condition variable pointed to by cv, if one exists.
 *
 *  @param cv The pointer to condition variable
 *  @return Zero on success, and a negative number on error
 */


int cond_signal( cond_t *cv ) {
  return wakeup_first_waiter(&cv->condWaitControl,NULL);
}


/** @brief This function wakes up all threads waiting on the
 *         condition variable pointed to by cv.
 *
 *  @param cv The pointer to condition variable
 *  @return Zero on success, and a negative number on error
 */

int cond_broadcast( cond_t *cv ) {
  return wakeup_all(&cv->condWaitControl);
}
