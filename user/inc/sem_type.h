/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <mutex.h> /*for mutex related functions and declarations*/
#include <assert.h>
#include <thread_lib_errno.h>
#include <wait_control_block.h>


typedef struct sem {
  int count;
  //- the wcb has a mutex with a list --//
  WAIT_CONTROL_BLOCK semWaitControlBlock;
} sem_t;

typedef sem_t *PSEM;


/*********Macro to initialize semaphore structure *****************/

#define SEM_INIT(pSem,cnt) do {					\
    pSem->count = (cnt);					\
    INIT_WAIT_CONTROL_BLCK( &pSem->semWaitControlBlock );	\
}while(0)

/*********Macro to lock semaphore mutex structure *****************/

#define lock_sem_mutex( pSem ) do {		                \
    int ret;							\
    ret = lock_wait_control_block(&pSem->semWaitControlBlock);	\
    assert( ret == ETHREAD_SUCCESS );				\
}while(0)

/*********Macro to unlock semaphore mutex structure *****************/

#define unlock_sem_mutex( pSem ) do {		                \
    int ret;					                \
    ret = unlock_wait_control_block(&pSem->semWaitControlBlock);\
    assert( ret == ETHREAD_SUCCESS );		                \
}while(0)


#endif /* _SEM_TYPE_H */
