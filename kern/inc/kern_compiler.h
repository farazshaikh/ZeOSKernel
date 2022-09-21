/** @file     kern_compiler.h
 *  @brief    This file contains special definitions/abstraction 
 *            extracted/exploited from the compiler 
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _KERN_COMPILER_H
#define _KERN_COMPILER_H

#define PACKED __attribute__((__packed__))
#define ASM_LINKAGE __attribute__((regparm(0)))
#define NORETURN __attribute__((regparm(0)))

#endif // _KERN_COMPILER_H
