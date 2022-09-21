/** @file     syscall_wait.c
 *  @brief    This file contains the system call handler for wait()
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86/seg.h>
#include <malloc.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"


/** @function  syscall_wait
 *  @brief     This function implements the wait system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */

KERN_RET_CODE syscall_wait(void *user_param_packet)  {
  int *user_status; 
  ktask *pTaskTrav=NULL;
  KERN_RET_CODE retval;

  user_status = (int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);

  task_fork_lock((CURRENT_THREAD)->pTask);

  // -- If parent never forked -- //
  if( (CURRENT_THREAD)->pTask->ktask_task_head.nr_elements == 0 ) {
    task_fork_unlock((CURRENT_THREAD)->pTask);
    return KERN_ERROR_TASK_NOT_FOUND;
  }

  task_fork_unlock((CURRENT_THREAD)->pTask);  

  //- there is no LOST SIGNAL problem here as semaphore remembers --//
  pTaskTrav = NULL;
  sem_wait(&(CURRENT_THREAD)->pTask->vultures);
  

  //- find the child task that exited --//
  task_fork_lock((CURRENT_THREAD)->pTask);
  Q_FOREACH( pTaskTrav , &(CURRENT_THREAD)->pTask->ktask_task_head , ktask_next ) {
    if(pTaskTrav->state == TASK_STATUS_ZOMIE)
      break;
  }

  //- someone woke us up so that someone has to be in zombie state -//
  assert( pTaskTrav->state == TASK_STATUS_ZOMIE );
  if( pTaskTrav->state != TASK_STATUS_ZOMIE ) {
    task_fork_unlock((CURRENT_THREAD)->pTask);
    return KERN_ERROR_TASK_NOT_FOUND;
  }
  
  //- Perform last rites and clean up -//
  Q_REMOVE( &(CURRENT_THREAD)->pTask->ktask_task_head, pTaskTrav , ktask_next );
  *user_status = pTaskTrav->status;
  
  retval = (KERN_RET_CODE)&pTaskTrav->initial_thread;
  
  // -- free up the space used by the waited task -- //
  vmm_free_task_vm(pTaskTrav);
  task_fork_unlock((CURRENT_THREAD)->pTask);	
  return retval;
}
