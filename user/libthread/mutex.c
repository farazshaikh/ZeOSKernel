/** @file mutex.c
 *  @brief This file defines the mutex functions.
 *
 *  @author Faraz Shaikh (fshaikh)  Deepak Amin (dvamin)
 *
 *
 */

#include <thread.h>
#include <malloc.h>
#include <thread_lib_errno.h>
#include <syscall.h>
#include <syscall_int.h>
#include <simics.h>
#include <mutex.h>

/** @brief This function initializes the mutex which is allocated at mp.
 *         The effects of using a mutex before it has been initialized,
 *         or of initializing it when it is already initialized and in use,
 *         are undefined.
 *
 *  @param mp - pointer to the mutex
 *  @return ETHREAD_SUCCESS on success,
 *          ETHREAD_FAILURE if mp is a dummy value
 */

int mutex_init( mutex_t *mp ) {
  if(NULL == mp)
    return ETHREAD_ERR;

  memset( mp , 0 , sizeof(*mp));
  return ETHREAD_SUCCESS;
}

/** @brief This function deactivates the mutex pointed to by mp.
 *         The effects of using a mutex from the time of its destruction
 *         until the time of a possible later re-initialization are undefined.
 *         If this function is called while the mutex is locked, it return an error.
 *
 *  @param mp - pointer to the mutex
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR if mp is a dummy value
 *          ETHREAD_BUSY if mp is locked by a thread
 */

int mutex_destroy( mutex_t *mp ) {
  if(NULL == mp)
    return ETHREAD_ERR;

  if( !mp->is_locked )
    return ETHREAD_BUSY;

  memset( mp , 0 , sizeof(*mp));
  return ETHREAD_SUCCESS;
}

/** @brief A call to this function ensures mutual exclusion in the region
 *         between itself and a call to mutex_unlock(). A thread calling
 *         this function while another thread is in an interfering critical
 *         section will yeild to another random thread as this way all other
 *         threads will have enough chance to run or acquire the mutex.
 *
 *  @param mp - pointer to the mutex
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR if mp is a dummy value
 */

int mutex_lock( mutex_t *mp ) {
  int __result;

  if(NULL == mp)
    return ETHREAD_ERR;


 retry:
  __asm__ (
    "mov 8(%%ebp),%%edx;\n\t"
    "mov $1,%%eax;\n\t"
    "xchg %%eax,(%%edx);\n\t"
    : "=a" (__result)
    :
    : "%edx"
    );

  // Retry //
  if(__result) {
    //if( 0 != yield(mp->thread_id))
      yield(-1);
    goto retry;
  }

  //-- We now have the lock -//
  mp->thread_id = gettid();
  return __result;
}

/** @brief This function signals the end of a region of mutual exclusion.
 *         The calling thread gives up its claim to the lock.
 *
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR if mp is a dummy value or
 *                        if thread thats not holding the mutex tries to unlock
 *                        (not sure when we encounter this case, but is given in handout)
 */


int mutex_unlock( mutex_t *mp ) {
  if(NULL == mp)
    return ETHREAD_ERR;


  mp->thread_id = 0;
  mp->is_locked = 0;
  return ETHREAD_SUCCESS;
}

/******************Helper Functions******************/

/** @brief This function is used to create a mutex as locked = 1
 *
 *  @param mp - pointer to the mutex
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_ERR if mp is a dummy value or
 */

int _mutex_init_locked( mutex_t *mp ) {
  if(NULL == mp)
    return ETHREAD_ERR;

  memset( mp , 0 , sizeof(*mp));
  mp->is_locked = 1;
  return ETHREAD_SUCCESS;
}

/** @brief This function is used to check if a mutex is locked
 *
 *  @param mp - pointer to the mutex
 *  @return lock status (1=locked, 0=free) on success
 *          ETHREAD_ERR if mp is a dummy value or
 */


int is_mutex_locked( mutex_t *mp ) {
  if(NULL == mp)
    return ETHREAD_ERR;

  return mp->is_locked;
}

