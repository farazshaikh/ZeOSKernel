/** @file     sched.h
 *  @brief    This file defines the scheduler structure and types
 *            macros for its initialization and exported function prototypes
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _SCHED_H
#define _SCHED_H
#include <kern_common.h>
#include <x86/page.h>
#include <task.h>

Q_NEW_HEAD( task_sched_head , kthread );

typedef struct _scheduler { 
  spinlock        scheduler_lock;
  int             preemption_disable_count;
  task_sched_head run_queue; 
  int             nr_context_switches;
}scheduler; 


#define INIT_SCHEDULER(pScheduler) do {			\
    (pScheduler)->preemption_disable_count = 0;		\
    SPINLOCK_INIT(&(pScheduler)->scheduler_lock);	\
    Q_INIT_HEAD(&(pScheduler)->run_queue);		\
    (pScheduler)->nr_context_switches=0;                \
}while(0)

KERN_RET_CODE sched_init();
#define  CURRENT_RUNNABLE     1
#define  CURRENT_NOT_RUNNABLE 0 
void schedule(int isCurrentRunnable); 
void scheduler_add(kthread *pkthread);
void scheduler_remove(kthread *thread);
void scheduler_timer_callback(unsigned int jiffies);

uint32_t disable_preemption(void);
void enable_preemption(uint32_t);

#endif 
