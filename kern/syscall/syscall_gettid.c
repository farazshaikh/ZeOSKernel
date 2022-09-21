/** @file     syscall_gettid.c
 *  @brief    This file contains the system call handler for get_tid()
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


/** @function  syscall_gettid
 *  @brief     This function implements the get_tid system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    tid (represented by the thread id) to the caller
 */

KERN_RET_CODE syscall_gettid(void *user_param_packet) { 
  return (KERN_RET_CODE) (CURRENT_THREAD);
}
