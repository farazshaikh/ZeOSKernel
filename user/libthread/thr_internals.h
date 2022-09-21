/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */


#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <mutex.h>
#include <dlink_list.h>
#include <string.h>
#include <wait_control_block.h>
#include <thread_lib_errno.h>
#include <cond.h>

/****Globally defined constants*****/
#define RUN_STATE_RUNNING  0
#define RUN_STATE_STOPPED -1
#define RUN_STATE_INVALID -2

#define ANY_THREAD        -1
#define THIS_THREAD        0


//-- the thread start function --//
typedef void *(*PTHREAD_START_FN)(void *);
typedef void *THREAD_EXIT_STATUS;


typedef enum _THREAD_STATE {
  THREAD_STATE_INIT,
  THREAD_STATE_RUNNABLE,
  THREAD_STATE_WAITING,
  THREAD_STATE_COMPLETED,
  THREAD_STATE_WAIT_COMPLETED
}THREAD_STATE;


//////////////////////////////
//-- thread control block --//
//////////////////////////////

typedef struct __THREAD_CNTRL_BLCK {
  THREAD_ID tid;
  unsigned int ostid;

  THREAD_STATE state;
  THREAD_EXIT_STATUS status;
  THREAD_EXIT_STATUS last_wait_thread_status;

  char *thread_stack_base;

  DLIST_ENTRY nextThreadInTask;
  cond_t       joinCondition;
  DLIST_ENTRY  nextWaitingThread;
} THREAD_CNTRL_BLCK,*PTHREAD_CNTRL_BLCK;

//////////////////////////////////////////////////
//-- macro to initialize thread control block --//
//////////////////////////////////////////////////

#define INIT_THREAD_CONTROL_BLCK(pThreadControlBlock,tsb) do {		\
    PTHREAD_CNTRL_BLCK _pThreadControlBlock = (pThreadControlBlock);    \
    memset ( _pThreadControlBlock , 0 , sizeof(*_pThreadControlBlock));	\
    _pThreadControlBlock->state  = THREAD_STATE_INIT;			\
    _pThreadControlBlock->status = NULL;				\
    _pThreadControlBlock->thread_stack_base = (tsb);			\
    DLIST_INIT( &_pThreadControlBlock->nextThreadInTask );		\
    INIT_COND_VAR( &_pThreadControlBlock->joinCondition ) ;		\
    DLIST_INIT( &_pThreadControlBlock->nextWaitingThread );		\
}while(0)


//////////////////////////////
//--  task control block  --//
//////////////////////////////
typedef struct __TASK_CNTRL_BLCK {
  int          threadLibInitialized;
  unsigned int nextthreadId;
  unsigned int threadStackSize;
  unsigned int primary_thread_ostid;

  mutex_t      anchorThrdsMutex;
  DLIST_ENTRY  anchorThrds;
}TASK_CNTRL_BLCK, *PTASK_CNTRL_BLCK;

//////////////////////////////////////////////////
//--  macro to initialize task control block  --//
//////////////////////////////////////////////////


#define INIT_TASK_CONTRL_BLCK(pTaskControlBlock,stackSize,pt_ostid)	\
  do {									\
    PTASK_CNTRL_BLCK _pTaskControlBlock = (pTaskControlBlock);	        \
    memset( _pTaskControlBlock , 0 , sizeof(*_pTaskControlBlock) );	\
    DLIST_INIT( &_pTaskControlBlock->anchorThrds );			\
    _pTaskControlBlock->threadStackSize = stackSize;			\
    _pTaskControlBlock->threadLibInitialized = 1;			\
    _pTaskControlBlock->primary_thread_ostid = (pt_ostid);		\
    mutex_init(&_pTaskControlBlock->anchorThrdsMutex);			\
}while(0)

/////////////////////////////////////////////////////
//--  other global helper function declarations  --//
/////////////////////////////////////////////////////

PTHREAD_CNTRL_BLCK getThreadControlBlock_ostid( unsigned int ostid );
PTHREAD_CNTRL_BLCK getThreadControlBlock( THREAD_ID tid );
void lockTaskControlBlock();
void unlockTaskControlBlock();
int isMutexThreadWorldLock(mutex_t *mp);
#endif /* THR_INTERNALS_H */
