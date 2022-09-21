/** @file     syscall_cas2irunflag.c
 *  @brief    This file contains the system call handler for cas2i_runflag()
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86/seg.h>
#include <malloc.h>
#include <x86/cr.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include <sched.h>


/** @function  syscall_cas2irunflag
 *  @brief     This function implements the cas2irunflag system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE syscall_cas2irunflag(void *user_param_packet) {
  uint32_t     eflags;
  KERN_RET_CODE ret = KERN_SUCCESS;
  int *oldp, tid, ev1, nv1, ev2, nv2;
  kthread *thisThread = (CURRENT_THREAD);
  kthread *targetThread;
  FN_ENTRY();

  // -- extract the arguments to the call -- //
  tid = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  oldp = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);
  ev1 = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,2);
  nv1 = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,3);
  ev2 = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,4);
  nv2 = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,5);


  // -- verify if the pointer references are correct -- //
  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (void *)oldp , sizeof(int) );
  if( KERN_SUCCESS != ret )
    return KERN_ERROR_GENERIC;

  targetThread = (kthread *)tid;

  // -- lock scheduler (feign atomicity) -- //
  eflags = disable_preemption();

  // -- extract the return value -- //
  *oldp = targetThread->run_flag;

  // -- set runflag to nv1 if it was ev1 -- //
  if( *oldp == ev1 ) {
    if( (nv1 < 0)&&( targetThread != thisThread ) ) {
      enable_preemption(eflags);
      return KERN_ERROR_GENERIC;
    }
    targetThread->run_flag = nv1;
  }

  // -- set runflag to nv2 if it was ev2 -- //
  if( *oldp == ev2 ) {
    if( (nv2 < 0)&&( targetThread != thisThread ) ) {
      enable_preemption(eflags);
      return KERN_ERROR_GENERIC;
    }
    targetThread->run_flag = nv2;
  }


  // -- unlock scheduler -- //
  enable_preemption(eflags);
  schedule(CURRENT_RUNNABLE);
  
  FN_LEAVE();
  return KERN_SUCCESS;
}
