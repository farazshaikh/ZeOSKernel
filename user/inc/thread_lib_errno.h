/** @file thread_lib_errno.h
 *  @brief This file defines most of the error codes
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#ifndef __THREAD_LIB_ERRNO__
#define __THREAD_LIB_ERRNO__

typedef int THREAD_ID;
#define ETHREAD_SUCCESS          0
#define ETHREAD_NO_MEM          -1
#define ETHREAD_NOT_IMPLEMENTED -2
#define ETHREAD_BUSY            -3
#define ETHREAD_ERR             -4
#define ETHREAD_NOT_FOUND       -5

/*Not all of below values are used,
  but this gives an idea about the datastructures and calls
  and also the possible errors that need to be handled*/

/*Return values during semaphore manipulations*/
#define ETHREAD_SEM_ERR         -6
#define ETHREAD_SEM_MUTEX_ERR ETHREAD_ERR
#define ETHREAD_SEM_THREAD_ERR  -7 /*cas2i_runflag failures*/
#define ETHREAD_SEM_DESTROY_ERR -8
#define ETHREAD_SEM_LIST_ERR    -9

/*Return values during cond var manipulations*/
#define ETHREAD_COND_ERR        -10
#define ETHREAD_COND_ARG_ERR    -11 /*if the mutex argument to cond_wait is not locked a priori*/
#define ETHREAD_COND_MUTEX_ERR  -12
#define ETHREAD_COND_THREAD_ERR -13 /*cas2i_runflag failures*/
#define ETHREAD_COND_DESTROY_ERR -14
#define ETHREAD_COND_LIST_ERR    -15


/*Return values during rwlock manipulations*/
#define ETHREAD_RWL_ERR         -16
#define ETHREAD_RWL_MUTEX_ERR   -17
#define ETHREAD_RWL_DESTROY_ERR -18
#define ETHREAD_RWL_LIST_ERR    -19


#endif // __THREAD_LIB_ERRNO_
