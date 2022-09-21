/** @file wait_control_block.h
 *
 *  @brief This file is used to define things
 *         internal to the wait control block handling.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */


#ifndef WAIT_CONTROL_BLOCK_H
#define WAIT_CONTROL_BLOCK_H

#include <thread_lib_errno.h>
#include <mutex.h>
#include <dlink_list.h>

/*Wait Control Block*/


typedef struct _WAIT_CONTROL_BLOCK {
  mutex_t     waiters_list_mutex;
  int         waiters_nr;
  DLIST_ENTRY waiters_anchor;
}WAIT_CONTROL_BLOCK, *PWAIT_CONTROL_BLOCK;

/*****Macro to initialize Wait Control Block****/


#define INIT_WAIT_CONTROL_BLCK(pWaitControlBlock) do {			\
    (pWaitControlBlock)->waiters_nr = 0;				\
    DLIST_INIT( &(pWaitControlBlock)->waiters_anchor ) ;		\
}while(0)


/******** Global Function Declarations ************/

int sleep_on_unprotected(PWAIT_CONTROL_BLOCK pWaitControlBlock);


int sleep_on_if_first(PWAIT_CONTROL_BLOCK pWaitControlBlock);
int sleep_on(PWAIT_CONTROL_BLOCK pWaitControlBlock);
int sleep_on_no_schedule(PWAIT_CONTROL_BLOCK pWaitControlBlock);


int wakeup_tid(PWAIT_CONTROL_BLOCK pWaitControlBlock,THREAD_ID tid, mutex_t *m);
int wakeup_all(PWAIT_CONTROL_BLOCK pWaitControlBlock);
int wakeup_first_waiter(PWAIT_CONTROL_BLOCK pWaitControlBlock,  mutex_t *m);
int wakeup_last_waiter(PWAIT_CONTROL_BLOCK pWaitControlBlock,   mutex_t *m);

int lock_wait_control_block(PWAIT_CONTROL_BLOCK pWaitControlBlock);
int unlock_wait_control_block(PWAIT_CONTROL_BLOCK pWaitControlBlock);

void thread_schedule(void);
#endif
