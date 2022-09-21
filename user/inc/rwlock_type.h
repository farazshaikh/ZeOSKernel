/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <wait_control_block.h>
#include <cond.h>
#include "thr_internals.h"

#define READ_LOCK   0
#define WRITE_LOCK  1 //-- Anything other than 0 --//

typedef struct rwlock {
  int            active_readers;
  THREAD_ID      active_writer_tid;
  cond_t readersCondVar;
  cond_t writersCondVar;
} rwlock_t;

/*********Macro to initialize rwlock structure *****************/
#define INIT_RW_LOCK( rwl ) do {			        \
    (rwl)->active_readers = 0;				        \
    (rwl)->active_writer_tid = 0;			        \
    INIT_COND_VAR(&(rwl)->readersCondVar);			\
    INIT_COND_VAR(&(rwl)->writersCondVar);			\
}while(0)

#endif /* _RWLOCK_TYPE_H */
