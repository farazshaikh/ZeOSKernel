/** @file wait_control_block.c
 *  @brief This file defines functions related to semaphore type
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#include <thread.h>
#include "thr_internals.h"
#include <malloc.h>
#include <thread_lib_errno.h>
#include <syscall.h>
#include <syscall_int.h>
#include <simics.h>


/** @brief The below macros are used to enter and exit the critical sections **/
//--- Wait blocks --//
#define WAIT_CS_START(pWaitControlBlock , ret ) do {		\
     (ret) = lock_wait_control_block((pWaitControlBlock));	\
     assert( (ret) == ETHREAD_SUCCESS );			\
}while(0)

#define WAIT_CS_END(pWaitControlBlock , ret ) do {		\
     (ret) = unlock_wait_control_block((pWaitControlBlock));	\
     assert( (ret) == ETHREAD_SUCCESS );			\
}while(0)


/** @brief sleep_on_unprotected - This function allows the caller to enter
 *         sleep/wait list without possessing any locks. Helper Function
 *         This function puts the thread in wait queue.
 *         It doesn't set the thread status at OS to WAITING (no cas2i)
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */

int sleep_on_unprotected(PWAIT_CONTROL_BLOCK pWaitControlBlock) {
  PTHREAD_CNTRL_BLCK pThis_thread;

  pThis_thread = getThreadControlBlock(THIS_THREAD);
  assert( NULL != pThis_thread );


  //-- We better not be already hooked on some other waiter --//
  assert(DLIST_EMPTY(&pThis_thread->nextWaitingThread));
  assert(pThis_thread->state == THREAD_STATE_RUNNABLE);

  dlist_add_tail(
		  &pWaitControlBlock->waiters_anchor,
		  &pThis_thread->nextWaitingThread
		  );

  pWaitControlBlock->waiters_nr++;
  return ETHREAD_SUCCESS;
}

/** @brief sleep_on_no_schedule - This function allows the caller to enter
 *         sleep/wait list after possessing the locks.
 *         This function puts the thread in wait queue.
 *         It doesn't set the thread status at OS to WAITING (no cas2i)
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */

int sleep_on_no_schedule(PWAIT_CONTROL_BLOCK pWaitControlBlock) {
  PTHREAD_CNTRL_BLCK pThis_thread;
  int ret;


  pThis_thread = getThreadControlBlock(THIS_THREAD);
  assert( NULL != pThis_thread );

  WAIT_CS_START(pWaitControlBlock,ret);
  //-- We better not be already hooked on some other waiter --//
  assert(DLIST_EMPTY(&pThis_thread->nextWaitingThread));
  assert(pThis_thread->state == THREAD_STATE_RUNNABLE);

  dlist_add_tail(
		  &pWaitControlBlock->waiters_anchor,
		  &pThis_thread->nextWaitingThread
		  );
  pWaitControlBlock->waiters_nr++;
  WAIT_CS_END(pWaitControlBlock,ret);
  return ETHREAD_SUCCESS;
}

/** @brief wakeup_first_waiter - This function allows the caller to enter
 *         sleep/wait list after possessing the locks.
 *         This function extracts the first thread in wait queue.
 *         It then schedules the thread using cas2i
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR on failure
 *          asserts on failure
 */

int wakeup_first_waiter(PWAIT_CONTROL_BLOCK pWaitControlBlock, mutex_t *release_mutex ) {
  int ret;
  int old_run_state;
  PTHREAD_CNTRL_BLCK pThis_thread;
  DLIST_ENTRY *temp;

  if( release_mutex )
    assert( is_mutex_locked (release_mutex) );

  WAIT_CS_START(pWaitControlBlock,ret);
  if(!DLIST_EMPTY(&pWaitControlBlock->waiters_anchor)) {
    //- Get the waiter of the list -//
    temp = dlist_pop_head(&pWaitControlBlock->waiters_anchor);
    assert(temp);
    DLIST_INIT(temp);
    pWaitControlBlock->waiters_nr--;

    //-- Schedule it --//
    pThis_thread = DLIST_CONTAINER(temp,THREAD_CNTRL_BLCK,nextWaitingThread);
    pThis_thread->state = THREAD_STATE_RUNNABLE;
    cas2i_runflag(pThis_thread->ostid,
		  release_mutex ? (int *)&release_mutex->is_locked:&old_run_state,
		  RUN_STATE_STOPPED,
		  RUN_STATE_RUNNING,
		  RUN_STATE_RUNNING,
		  RUN_STATE_RUNNING);
    WAIT_CS_END(pWaitControlBlock,ret);
    return ETHREAD_SUCCESS;
  }else {
    if(release_mutex)
      mutex_unlock(release_mutex);

    assert(DLIST_EMPTY(&pWaitControlBlock->waiters_anchor));
    WAIT_CS_END(pWaitControlBlock,ret);
    return ETHREAD_ERR;
  }
  assert(0);
  return ret;
}

/** @brief wakeup_last_waiter - This function allows the caller to enter
 *         sleep/wait list after possessing the locks.
 *         This function extracts the last thread in wait queue.
 *         It then schedules the thread using cas2i
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR on failure
 *          asserts on failure
 */

int wakeup_last_waiter(PWAIT_CONTROL_BLOCK pWaitControlBlock , mutex_t *release_mutex) {
  int ret;
  int old_run_state;
  PTHREAD_CNTRL_BLCK pThis_thread;
  DLIST_ENTRY *temp;

  if( release_mutex )
    assert( is_mutex_locked (release_mutex) );

  WAIT_CS_START(pWaitControlBlock,ret);
  if(!DLIST_EMPTY(&pWaitControlBlock->waiters_anchor)) {
    //- Get the waiter of the list -//
    temp = dlist_pop_tail(&pWaitControlBlock->waiters_anchor);
    assert(temp);
    DLIST_INIT(temp);
    pWaitControlBlock->waiters_nr--;


    //-- Schedule it --//
    pThis_thread = DLIST_CONTAINER(temp,THREAD_CNTRL_BLCK,nextWaitingThread);
    pThis_thread->state = THREAD_STATE_RUNNABLE;
    cas2i_runflag(pThis_thread->ostid,
		  release_mutex ? (int *)&release_mutex->is_locked:&old_run_state,
		  RUN_STATE_STOPPED,
		  RUN_STATE_RUNNING,
		  RUN_STATE_INVALID,
		  RUN_STATE_INVALID);
    WAIT_CS_END(pWaitControlBlock,ret);
    return ETHREAD_SUCCESS;
  }else {
    if(release_mutex)
      mutex_unlock(release_mutex);

    WAIT_CS_END(pWaitControlBlock,ret);
    return ETHREAD_ERR;
  }

  assert(0);
  return ret;
}

/** @brief wakeup_tid (not used here - or future work)
 *         This function allows the caller to wakeup a thread
 *         by referring to its tid.
 *         This function extracts the first thread in wait queue.
 *         It then schedules the thread using cas2i
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @param  tid - thread ID assigned by the thread library
 *  @param  release_mutex - mutex on which the thread is waiting
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR on failure
 *          asserts on failure
 */

int wakeup_tid(PWAIT_CONTROL_BLOCK pWaitControlBlock , THREAD_ID tid , mutex_t *release_mutex) {
  int ret;
  WAIT_CS_START(pWaitControlBlock,ret);


  WAIT_CS_END(pWaitControlBlock,ret);
  return ret;
}

/** @brief wakeup_all - This function allows the caller to enter
 *         sleep/wait list after possessing the locks.
 *         This function extracts all the threads in wait queue.
 *         It then schedules each one of them using cas2i
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR on failure
 *          asserts on failure
 */

int wakeup_all(PWAIT_CONTROL_BLOCK pWaitControlBlock) {
  int ret;
  int old_run_state;
  PTHREAD_CNTRL_BLCK pThis_thread;
  DLIST_ENTRY *temp;
  DLIST_ENTRY  new_head;


  WAIT_CS_START(pWaitControlBlock,ret);
  if(pWaitControlBlock->waiters_nr) {
    assert( !DLIST_EMPTY(&pWaitControlBlock->waiters_anchor) );
    temp = pWaitControlBlock->waiters_anchor.next;

    //-- Detach the head from the list now --//
    pWaitControlBlock->waiters_anchor.next->prev = pWaitControlBlock->waiters_anchor.prev;
    pWaitControlBlock->waiters_anchor.prev->next = pWaitControlBlock->waiters_anchor.next;
    DLIST_INIT(&pWaitControlBlock->waiters_anchor);
    pWaitControlBlock->waiters_nr = 0;
    WAIT_CS_END(pWaitControlBlock,ret);

    //-- Attach a new head as its easy to user pop-push from a head --//
    DLIST_INIT(&new_head);
    dlist_attach_new_head(&new_head,temp);


    while(!DLIST_EMPTY(&new_head)) {
      //- Get the waiter of the list -//
      temp = dlist_pop_tail(&new_head);
      assert(temp);
      DLIST_INIT(temp);

      //-- Schedule it --//
      pThis_thread = DLIST_CONTAINER(temp , THREAD_CNTRL_BLCK , nextWaitingThread);
      pThis_thread->state = THREAD_STATE_RUNNABLE;
      cas2i_runflag(pThis_thread->ostid,
		    &old_run_state,
		    RUN_STATE_STOPPED,
		    RUN_STATE_RUNNING,
		    RUN_STATE_INVALID,
		    RUN_STATE_INVALID);
    }
    return ETHREAD_SUCCESS;
  }else {
    WAIT_CS_END(pWaitControlBlock,ret);
    return ETHREAD_ERR;
  }

  assert(0);
  return ret;
}


/*********************Helper Functions**********************/

/** @brief lock_wait_control_block - This function is used if a thread
 *         wants to visit the sleep/wait list associated with a WCB member
 *         This function is used to lock the queue and exter the critical section.
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */


int lock_wait_control_block(PWAIT_CONTROL_BLOCK pWaitControlBlock) {
  int ret;
  ret = mutex_lock( &pWaitControlBlock->waiters_list_mutex );
  assert(ret == ETHREAD_SUCCESS);
  return ret;
}


/** @brief unlock_wait_control_block - This function is used if a thread
 *         wants to visit the sleep/wait list associated with a WCB member
 *         This function is used to unlock the queue and exter the critical section.
 *
 *
 *  @param  pWaitControlBlock - pointer to WaitControlBlock
 *  @return ETHREAD_SUCCESS on success
 *          asserts on failure
 */

int unlock_wait_control_block(PWAIT_CONTROL_BLOCK pWaitControlBlock) {
  int ret;
  ret = mutex_unlock( &pWaitControlBlock->waiters_list_mutex );
  assert(ret == ETHREAD_SUCCESS);
  return ret;
}



