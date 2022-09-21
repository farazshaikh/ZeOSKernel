/** @file     syscall_halt.c
 *  @brief    This file contains the system call handler for halt()
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
#include <exec2obj.h>
#include <elf_410.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include "bootdrvlib/timer_driver.h"
#include "bootdrvlib/keyb_driver.h"

#define HALT_STRING "Halting kernel ......"

/** @function  syscall_halt
 *  @brief     This function implements the halt system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    On success, this will never return
 */

KERN_RET_CODE syscall_halt(void *user_param_packet)  { 
  FN_ENTRY();

  putbytes(HALT_STRING,strlen(HALT_STRING));

  __asm__ __volatile__(
    "cli;\n\t"
    "hlt;\n\t"
      :
      :
		       );

  // -- if we have a sound in our kernel we'd play Zombie (Cranberries) -- //
  while(1)
    lprintf("Its in your head, Its in your heyead ZOMBIE ZOMBIE ZOMBIEYE.AYE.AYE");

  FN_LEAVE();
  return KERN_ERROR_GENERIC;
}
