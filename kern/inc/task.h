/** @file     task.h
 *  @brief    This file defines the structure and types of Task
 *            and Thread Control Blocks, and defines several prototypes
 *            which can be used to access the Tasks and Threads
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _TASK_H
#define _TASK_H
#include <kern_common.h>
#include <x86/page.h>
#include <sync.h>

#define INITIAL_BINARY       "init"
#define KTHREAD_KSTACK_PAGES 2
#define KTHREAD_KSTACK_SIZE  (KTHREAD_KSTACK_PAGES * PAGE_SIZE)UL
#define KTHREAD_USTACK_PAGES 2

#define CURRENT_THREAD stacked_current()
static inline struct kthread * stacked_current(void)
{
  struct kthread *current;
  __asm__(
	  "andl %%esp,%0; ":
	  "=r" (current) : 
	  "0" (~(8191UL))
	  );
  return current;
}


Q_NEW_HEAD( task_kthread_head , kthread );
Q_NEW_HEAD( task_ktask_head , ktask );     // for children of a task //

typedef enum _kthread_state { 
  kthread_runnable,
  kthread_waiting
}kthread_state;

typedef struct _kthread_ctx { 
  STACK_ELT *kstack;
  STACK_ELT *r_esp;
}kthread_ctx;


struct ktask;

// -- Thread Struct -- //

typedef struct kthread {
  kthread_ctx   context;
  ktask        *pTask;           // Scheduler needs its for  //
                                 //conditionally loading pdbr//
  Q_NEW_LINK( kthread ) kthread_next;
  Q_NEW_LINK( kthread ) kthread_wait;

  kthread_state state;
  int           sleepticks;
  int           run_flag;
}kthread; 


typedef  struct task_vm task_vm;

#define TASK_STATUS_ZOMIE  0xDEADBEEF

// -- Task Struct -- //
#define ALLOC_MEM_QUOTA  (512*1024*1024)
struct ktask {
  kthread           initial_thread; //- has to be first element -//
  task_vm           vm;

  task_kthread_head ktask_threads_head; //- all threads in this task-//

  //-- parent child relationship -//
  semaphore         fork_lock;          //- binary semaphore guards forks -//
  task_ktask_head   ktask_task_head;    //- all children           --//
  Q_NEW_LINK(ktask) ktask_next;         //- siblings-//
  struct   ktask   *parentTask;         //-parent task pointer//

  semaphore         vultures;           //- vultures waiting for task to die-//
  int               state;              //- currently we have only 1 state -//
  int               status;
  unsigned long     allocated_pages_mem; //- we have to have a quota for newpages_test to pass
}; 

// -- process synchronization primitives between tasks -- //
#define task_fork_lock(pTask)  sem_wait(&(pTask)->fork_lock)
#define task_fork_unlock(pTask)   sem_signal(&(pTask)->fork_lock)

// -- Function prototypes -- //
KERN_RET_CODE task_init(char *initial_binary); 
extern uint32_t get_esp(void);
extern uint32_t get_ebp(void);

int is_idle_thread(); 
kthread * get_idle_thread();

void thread_setup_iret_frame(kthread   *thread,
			     STACK_ELT  user_stack,
			     STACK_ELT  retIP,
			     STACK_ELT  error_code
			     );

void thread_setup_ret_from_syscall(
				   kthread      *thread,
				   STACK_ELT     user_stack,
				   STACK_ELT     retIP,
				   STACK_ELT     error_code,
				   i386_context *sysenter_user_context,
				   i386_context *context_switch_context
				   ) ;

KERN_RET_CODE sleep_init();
KERN_RET_CODE sleep_bottom_half();
#endif // _TASK_H
