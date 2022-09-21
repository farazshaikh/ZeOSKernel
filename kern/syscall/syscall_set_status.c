/** @file     syscall_set_status.c
 *  @brief    This file contains the system call handler for set_status()
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

/** @function  syscall_set_status
 *  @brief     This function implements the set_status system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE syscall_set_status(void *user_param_packet) { 
  DUMP("set_status to %p",(int) user_param_packet);
  // -- Set the argument in %esi as the status value of the task -- //
  CURRENT_THREAD->pTask->status = (int ) user_param_packet;
  return KERN_SUCCESS;
}
