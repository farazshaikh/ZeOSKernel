/** @file thread_lib.c
 *  @brief This file contains the main definitions of thread library
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */

#include <thread.h>
#include <malloc.h>
#include <thread_lib_errno.h>
#include <syscall.h>
#include <syscall_int.h>
#include <simics.h>

/******Mem alignment macros*********/
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define PAGE_ROUND (PAGE_SIZE - 1)

extern void vanish(void) NORETURN;
extern void thr_exit(void *);


/*******Global Declaration********/

PTASK_CNTRL_BLCK   pTaskControlBlock;
PTHREAD_CNTRL_BLCK pMainThreadControlBlock;

/**********Functions that return global var settings**********/

PTASK_CNTRL_BLCK getTaskControlBlock(){
  return pTaskControlBlock;
}
PTHREAD_CNTRL_BLCK getMainThreadControlBlock(){
  return pMainThreadControlBlock;
}


/*******************Helper Global Functions****************/

/** @brief getThreadControlBlock_ostid is a helper function
 *         that takes in the ostid = gettid() and returns the pointer to tCB
 *
 *  @param ostid - tid assigned by the OS.
 *  @return pointer to tCB structure.
 *          NULL on failure
 */

PTHREAD_CNTRL_BLCK getThreadControlBlock_ostid( unsigned int ostid ) {
  PTHREAD_CNTRL_BLCK pThreadControlBlock;
  PTASK_CNTRL_BLCK   pTaskControlBlock;

  //-- Special case for the primary thread -//
  if(ostid == getMainThreadControlBlock()->ostid)
    return getMainThreadControlBlock();

  pTaskControlBlock = getTaskControlBlock();

  //- Search list of child threads         -//
  FOR_EACH_CONTAINER(&pTaskControlBlock->anchorThrds,
		     pThreadControlBlock,
		     nextThreadInTask) {
    assert(NULL != pThreadControlBlock);

    if( pThreadControlBlock->ostid == ostid )
      return pThreadControlBlock;
  }
  return NULL;
}


/** @brief getThreadControlBlock is a helper function
 *         that takes in the thread id assigned by the thread library
 *         thr_getid() and returns the pointer to tCB
 *
 *  @param tid - tid assigned by the thread library.
 *  @return pointer to tCB structure.
 *          NULL on failure
 */

PTHREAD_CNTRL_BLCK getThreadControlBlock( THREAD_ID tid ) {
  PTHREAD_CNTRL_BLCK pThreadControlBlock;

  /*
  //-- Special case for the primary thread -//
  if(tid == getMainThreadControlBlock()->tid)
    return getMainThreadControlBlock();
  */

  //-- Special case handleing for THIS_THREAD for all except main thread-//
  if( tid == THIS_THREAD  ) {
    //-- Get it off the stack --//
    return getThreadControlBlock_ostid( gettid() );
  }

  //-- Ideally since our thread control block is on our own stack                --//
  //-- We should be able to get it by some aritmetic on our esp value            --//
  //-- I tried hard to implement this optimization bailed out as it crashed at times-//
  //-- Problem is with thread stack sizes that are not a multiple of power of 2 pages --//
  //-- Example 3Pages stack size calulation would be off by 1 PAGE               --//

  //- Search list of child threads         -//
  FOR_EACH_CONTAINER(&getTaskControlBlock()->anchorThrds,
		     pThreadControlBlock,
		     nextThreadInTask) {
    assert(NULL != pThreadControlBlock);

    if( pThreadControlBlock->tid == tid )
      return pThreadControlBlock;
  }
  return NULL;
}

/**************Mutex lock/unlock helper functions****************/
int isMutexThreadWorldLock(mutex_t *mp) {
  return (mp == &getTaskControlBlock()->anchorThrdsMutex);
}

void lockTaskControlBlock() {
  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);
}

void unlockTaskControlBlock() {
  mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
}

/**************End of helper functions****************/

/** @brief __thr_create is a system call implementation of thread create function
 *
 *  @param func - function to be executed by created thread
 *  @param args - args passedto that thread.
 *  @param *child_esp - stack pointer base of the child.
 *
 *  @return 0 to child
 *          child's tid to parent
 *
 */

int __thr_create( void *(*func)(void *), void *args, char *child_esp ) {
  int __result;
  __asm__(
	  "push %%ebx;"
	  "push %%edx;"
	  "push %%ecx;"

	  "mov  0x8(%%ebp),%%ebx;"
	  "mov  0xc(%%ebp),%%ecx;"
	  "mov  0x10(%%ebp),%%edx;"
	  :
	  :
	  );

  __asm__(
	  "int $" TO_STRING(THREAD_FORK_INT) ";\n\t"
	  "cmp $0x0,%%eax;"
	  "je  child;"

  "parent:"
	  "pop %%ecx;"
	  "pop %%edx;"
	  "pop %%ebx;"
	  "jmp ret_parent;"

  "child:"
	  "mov  %%edx,%%esp;"
	  "push %%ecx;"
	  "xor  %%ebp,%%ebp;"
	  "call %%ebx;"
	  "push %%eax;"
	  "call thr_exit;"

  "ret_parent:"

	  :"=a" (__result)
	  :
	  );
    return __result;
}



/** @brief thr_create is the wrapper C call on top of __thr_create
 *
 *  @param func - function to be executed by created thread
 *  @param args - args passedto that thread.
 *  @param *child_esp - stack pointer base of the child.
 *
 *  @return 0 to child
 *          child's tid to parent
 *          asserts on failure
 */

int thr_create( void *(*func)(void *), void *args ) {
  int ret = ETHREAD_NOT_IMPLEMENTED;
  THREAD_CNTRL_BLCK *pThreadControlBlock = NULL;
  char   *thread_stack_base;
  char   *thread_stack_end;
  char   *thread_esp;

  thread_stack_base = malloc( getTaskControlBlock()->threadStackSize );
  if( NULL == thread_stack_base )
    return ETHREAD_NO_MEM;
  thread_stack_end  = thread_stack_base + getTaskControlBlock()->threadStackSize - 1;


  //-- Round down to page boundary --//
  pThreadControlBlock = (PTHREAD_CNTRL_BLCK) ((unsigned long)thread_stack_end & ~PAGE_ROUND);
  pThreadControlBlock -= 1;

  //-- Initialize thread data-structures  --//
  INIT_THREAD_CONTROL_BLCK( pThreadControlBlock , thread_stack_base );

  thread_esp = (char *)(pThreadControlBlock - 1);


  //-- Very Very fair assumption to make --//
  assert(0 == ((unsigned long)thread_esp % 4));


  //-- Finally call the pebbles interface --//
  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);
  ret = __thr_create( func , args , thread_esp );


  //-- The child never returns from the above fn here  --//
  assert( ret != 0 );

  //-- The error case --//
  if( 0 > ret ) {
    mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
    free(pThreadControlBlock);
    return ret;
  }

  //-- the parent thread case --//
  if( 0 < ret ) {
    pThreadControlBlock->ostid = ret;

    do {
      pThreadControlBlock->tid   = getTaskControlBlock()->nextthreadId++;
    } while(pThreadControlBlock->tid == ANY_THREAD || pThreadControlBlock->tid == THIS_THREAD);

    assert(pThreadControlBlock->tid != ANY_THREAD && pThreadControlBlock->tid != THIS_THREAD);

    pThreadControlBlock->state = THREAD_STATE_RUNNABLE;

    dlist_push_head(&getTaskControlBlock()->anchorThrds,
    		    &pThreadControlBlock->nextThreadInTask);

    mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
    return pThreadControlBlock->tid;
  }

  assert(0);
  return pThreadControlBlock->tid; //-- Compiler complains --//
}




/** @brief thr_init() initializes the thread library and its components.
 *
 *
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_NO_MEM on memory issues
 *          asserts on failure
 */

int thr_init( unsigned int size ) {
  pTaskControlBlock = malloc( sizeof(*pTaskControlBlock)
			      + sizeof(*pMainThreadControlBlock) );
  if( NULL == pTaskControlBlock )
    return ETHREAD_NO_MEM;

  pMainThreadControlBlock = (PTHREAD_CNTRL_BLCK)
                             (pTaskControlBlock + 1);

  //-- The thread control block is the fist this thing to go on the --//
  //-- Thread stack                                                 --//
  size += sizeof(THREAD_CNTRL_BLCK) * 2;

  //-- Round up to PAGE_SIZE --//
  size += PAGE_ROUND;
  size &= ~PAGE_ROUND;

  //-- Malloc may not return PAGE_ALIGNED address --//
  size += PAGE_SIZE;

  INIT_TASK_CONTRL_BLCK(getTaskControlBlock(),size,gettid());
  INIT_THREAD_CONTROL_BLCK(pMainThreadControlBlock,NULL);
  getMainThreadControlBlock()->ostid = gettid();
  getMainThreadControlBlock()->tid   = THIS_THREAD + 1;
  getMainThreadControlBlock()->state = THREAD_STATE_RUNNABLE;  //- SPC boot strap case -//
  getTaskControlBlock()->nextthreadId= THIS_THREAD + 10;


  //-- Attach the main thread to the task --//
  dlist_push_head(&getTaskControlBlock()->anchorThrds,
		  &getMainThreadControlBlock()->nextThreadInTask);

  return ETHREAD_SUCCESS;
}


/** @brief thr_join() either blocks if the tid thread is active.
 *         or will join and reap tid thread
 *
 *  @param tid - tid of the join thread
 *  @param **statusp - pointer to void* status message holder between threads
 *
 *  @return ETHREAD_SUCCESS on success
 *          ETHREAD_BUSY is the thread is already joining with another
 *          ETHREAD_NOT_FOUND if the tid doesn't correspond to any tCB in the list.
 *          asserts on failures
 */


int thr_join( int tid, void **statusp ) {
  PTHREAD_CNTRL_BLCK pThreadControlBlock;
  PTHREAD_CNTRL_BLCK pSelfThreadControlBlock;

  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);


  //- Get the thread control blocks --//
  pThreadControlBlock = getThreadControlBlock(tid);

  pSelfThreadControlBlock = getThreadControlBlock(THIS_THREAD);
  assert(pSelfThreadControlBlock);


    //-- Thread to sleep on not found return error code -//
  if( NULL == pThreadControlBlock ) {
    assert(0);
    mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
    return ETHREAD_NOT_FOUND;
  }

  //- Are we having a joiner already --//
  if(!DLIST_EMPTY(&pThreadControlBlock->joinCondition.condWaitControl.waiters_anchor)){
    assert(0);
    mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
    return ETHREAD_BUSY;
  }



  //-- The thread hasn't already exited --//
  if(pThreadControlBlock->state != THREAD_STATE_COMPLETED) {
    cond_wait(&pThreadControlBlock->joinCondition,&getTaskControlBlock()->anchorThrdsMutex);
  }


  //-- Sure that thread has exited Cleanups --//
  assert(pThreadControlBlock);
  //-- Child completed --//
  assert(pThreadControlBlock->state == THREAD_STATE_COMPLETED);
  //-- Is child removed from list of active thread --//
  dlist_remove_entry(&pThreadControlBlock->nextThreadInTask);


  //-- Copy the threads return status --//
  if(NULL != statusp)
    *statusp = pThreadControlBlock->status;


  //-- Frees up stack AND the TCB that was created on the stack --//
  free(pThreadControlBlock->thread_stack_base);
  assert(DLIST_EMPTY(&pSelfThreadControlBlock->nextWaitingThread));


  mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
  return(ETHREAD_SUCCESS);
}

/** @brief thr_exit() is called by a function when it wants to stop its execution
 *
 *  @param status - return status pointer.
 *  @return void, but asserts on failure
 */

void thr_exit( void *status ) {
  PTHREAD_CNTRL_BLCK pThreadControlBlock;

  //-- Get our selves out of the list of active thread --//
  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);

  pThreadControlBlock = getThreadControlBlock(THIS_THREAD);
  assert(pThreadControlBlock);

  assert(!DLIST_EMPTY(&pThreadControlBlock->nextThreadInTask));
  pThreadControlBlock->status = status;
  pThreadControlBlock->state = THREAD_STATE_COMPLETED;

  //-- Wake up joiners if any --//
  cond_signal(&pThreadControlBlock->joinCondition);

  mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);

  //-- Vanish now --//
  vanish();
}


/** @brief thr_getid() returns the threadID of the currently thread.
 *
 *  @param status - void
 *  @return void, but asserts on failure
 */

int thr_getid( void ) {
  int ret = ETHREAD_NOT_FOUND;
  PTHREAD_CNTRL_BLCK pThreadControlBlock;
  unsigned int ostid;
  ostid = gettid();

  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);
  pThreadControlBlock = getThreadControlBlock_ostid(ostid);
  if(NULL != pThreadControlBlock)
    ret = pThreadControlBlock->tid;
  mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);

  return ret;
}

/** @brief thr_yield() makes the thread yield (suspend) itself
 *         and go on to schedule another process specified in the argument.
 *
 *  @param tid - thread that will take over operation from current tid
 *  @return asserts on failure
 */

int thr_yield( int tid ) {
  PTHREAD_CNTRL_BLCK pThreadControlBlock;
  unsigned int ostid;

  //-- Special Case --//
  if( tid == ANY_THREAD )
    return yield( ANY_THREAD );


  mutex_lock(&getTaskControlBlock()->anchorThrdsMutex);

  pThreadControlBlock = getThreadControlBlock(tid);
  if(NULL != pThreadControlBlock) {
    ostid = pThreadControlBlock->ostid;
  }else {
    mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
    return -ETHREAD_NOT_FOUND;
  }

  //-- call yeild for the underlying os thread --//
  mutex_unlock(&getTaskControlBlock()->anchorThrdsMutex);
  return yield(ostid);
}


