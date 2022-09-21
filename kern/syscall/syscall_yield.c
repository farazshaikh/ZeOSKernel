/** @file syscall_sleep.c 
 *
 *  @brief  Implementaion of yield system call
 *
 *  @author Faraz Shaikh (fshaikh)
 *  @bug No known bugs
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


/* The documentation says yield yields to a TARGET Thread 
   We believe that that target Thread's priority should not be moved-   
   This is because then we can have 2 threads being 2 nice to each other
   and bumping up each others priority  B->yield(A)  A->yield(B)        

   Instead what we do is reduce the priority of the CALLING thread to be 
   less than than that of the target thread. There by making sure target
   thread is run prior to the current thread.

   NOTE:
   Our mutex implementation is a spinlock. On a uniprocessor its equivalent
   of CLI and STI. We have not specific need for yeilding to a particular thead
   because our CS's never exit with spinlocks held !! (so there is no one to 
   yield to in a spinlock acquisition failure, since it never fails on uniprocessor)

   Albiet:
   The target thread may be sleeping not on the RUN-QUEUE but on *some* semaphore. 
   In this case current thread may be scheduled before target thread. That has
   to be handled by the thread that called yield. (the yield contract in spec in
   does not make a contrary commitment either)
*/


KERN_RET_CODE syscall_yield(void *user_param_packet) {
  kthread *thread;
  FN_ENTRY();

  thread = (kthread *) GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  if( thread->run_flag < 0 )
    return KERN_ERROR_GENERIC;

  schedule(CURRENT_RUNNABLE);

  FN_LEAVE();
  return KERN_SUCCESS;
}

