/** @file     kern_common.h
 *  @brief    This file contains a list of include files 
 *            common to several feature implementations in kernel
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef __KERNEL_COMMON_H_
#define __KERNEL_COMMON_H_

#define OS_NAME "InCarnate OS: 0.0.1\n"\
                "Faraz Shaikh, Deepak Amin\n"\
                "Carnegie Mellon University - 15410\n"

#include <common_kern.h>
#include <stdint.h>

#include <kern_compiler.h>
#include <kern_err.h>
#include <debuglib/kern_debug.h>
#include <syscall_entry.h>
#include <variable_queue.h>
#include <bootdrivers.h>
#include <types.h>

#include <i386lib/i386systemregs.h>
#include <i386lib/i386saverestore.h>

#include <vmm.h>
#include <task.h>
#include <sched.h>
#include <console.h>
#include <faulthandlers.h>
#include <loader_internal.h>
#include <sync.h>

void malloc_init();

#endif // __KERNEL_COMMON_H_



