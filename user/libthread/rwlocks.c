#include <rwlock.h>
#include <thread.h>
#include <cond.h>
#include <syscall.h> /*for cas2i_runflag*/
#include "thr_internals.h"

#define RW_INTERNAL_LOCK( rwl )						\
  mutex_lock( & (rwl)->readersCondVar.condWaitControl.waiters_list_mutex)

#define RW_INTERNAL_UNLOCK( rwl )					\
  mutex_unlock( & (rwl)->readersCondVar.condWaitControl.waiters_list_mutex)


#define RW_INTERNAL_LOCK_ADDR( rwl )					\
  ( & (rwl)->readersCondVar.condWaitControl.waiters_list_mutex)


int rwlock_init( rwlock_t *rwlock ) {
  INIT_RW_LOCK(rwlock);
  return ETHREAD_SUCCESS;
}

int _rwlock_lock_write( rwlock_t *rwlock) {

  RW_INTERNAL_LOCK(rwlock);
  if(rwlock->active_writer_tid || rwlock->active_readers) {
    cond_wait(&rwlock->readersCondVar,RW_INTERNAL_LOCK_ADDR(rwlock));
  }
  assert(0 == rwlock->active_writer_tid && 0 == rwlock->active_readers);
  rwlock->active_writer_tid = gettid();
  RW_INTERNAL_UNLOCK(rwlock);
  
  return ETHREAD_SUCCESS;
}


int _rwlock_lock_read( rwlock_t *rwlock) {

  RW_INTERNAL_LOCK(rwlock);
  if(rwlock->active_writer_tid != 0) {
    cond_wait(&rwlock->readersCondVar,RW_INTERNAL_LOCK_ADDR(rwlock));
  }
  assert(0 == rwlock->active_writer_tid);
  rwlock->active_readers++;
  RW_INTERNAL_UNLOCK(rwlock);

  return ETHREAD_SUCCESS;
}


int rwlock_lock( rwlock_t *rwlock, int type ) {
  if(READ_LOCK == type) {
    return _rwlock_lock_read(rwlock);
  } 
  return _rwlock_lock_write(rwlock);
}


int rwlock_unlock( rwlock_t *rwlock ) {
  RW_INTERNAL_LOCK(rwlock);
  THREAD_ID tid = thr_getid();

  //-- update availibility --//
  if(rwlock->active_writer_tid != 0 && 
     tid == rwlock->active_writer_tid) {
    //-- Assert no active readers while release of write --//
    assert(rwlock->active_readers == 0);
    rwlock->active_writer_tid = 0;
  }else {
    //- Assert no writers active while release of read -//
    assert(rwlock->active_writer_tid == 0);
    rwlock->active_readers--;
  }
    

  //-- Read's can be given out after any unlock ie. R/W unlock --//
  if(rwlock->readersCondVar.condWaitControl.waiters_nr) {
    cond_broadcast(&rwlock->readersCondVar);
    RW_INTERNAL_UNLOCK(rwlock);
    return ETHREAD_SUCCESS;
  }

  //-- Write unlock given if 
  //-- No reader waiting
  //-- No reader active
  //-- Someone wants to write 
  if(rwlock->readersCondVar.condWaitControl.waiters_nr && rwlock->active_readers == 0) {
    cond_signal(&rwlock->writersCondVar);
    RW_INTERNAL_UNLOCK(rwlock);
    return ETHREAD_SUCCESS;
  }

  RW_INTERNAL_UNLOCK(rwlock);
  return ETHREAD_SUCCESS;
}




int rwlock_destroy( rwlock_t *rwlock ) {
  RW_INTERNAL_LOCK(rwlock);

  if(rwlock->active_readers || rwlock->active_writer_tid) {
    RW_INTERNAL_UNLOCK(rwlock);
    return ETHREAD_BUSY;
  }

  if(rwlock->readersCondVar.condWaitControl.waiters_nr ||
     rwlock->writersCondVar.condWaitControl.waiters_nr ) {
    RW_INTERNAL_UNLOCK(rwlock);
    return ETHREAD_BUSY;
  }

  memset(rwlock,0,sizeof(*rwlock));
  return ETHREAD_SUCCESS;
}
