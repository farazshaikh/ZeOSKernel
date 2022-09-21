/** @file     syscall_getticks.c
 *  @brief    This file contains the system call handler for get_ticks()
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
#include "bootdrvlib/timer_driver.h"


/** @function  syscall_getticks
 *  @brief     This function implements the getticks system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    timer_ticks to caller
 */

KERN_RET_CODE syscall_getticks(void *user_param_packet) {
  return timer_get_ticks();
}
