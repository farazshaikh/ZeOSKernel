/** @file     syscall_entry.h
 *  @brief    This file defines the interface for initializing 
 *            the system call handler sub system
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _SYS_CALL_H
#define _SYS_CALL_H

#include <kern_common.h>

KERN_RET_CODE syscall_init(void); 

#endif // _SYS_CALL_H
