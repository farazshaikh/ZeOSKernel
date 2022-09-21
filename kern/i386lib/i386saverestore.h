/** @file     i386saverestore.h
 *  @brief    This file contains the implementation of macros and types
 *            exported to the files in the i386lib folder
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef __SAVE_RESTORE
#define __SAVE_RESTORE

#define CONTEXT_REGS_NR 9


#ifdef ASSEMBLER
#define SAVE_REGS 		\
	pushl %ebx		;\
	pushl %ecx		;\
	pushl %edx		;\
	pushl %esi		;\
	pushl %edi		;\
	pushl %es		;\
	pushl %ds		;\
	pushl %ebp		;\
	pushl %eax



#define RESTORE_REGS		\
	pop %eax		;\
	pop %ebp                ;\
	pop %ds			;\
	pop %es			;\
	pop %edi		;\
	pop %esi		;\
	pop %edx		;\
	pop %ecx		;\
	pop %ebx

#else // gcc inline assembly versions //

#include <kern_compiler.h>
#include "i386lib/i386systemregs.h"

#define SAVE_REGS 		\
	"pushl %%ebx		;"\
	"pushl %%ecx		;"\
	"pushl %%edx		;"\
	"pushl %%esi		;"\
	"pushl %%edi		;"\
	"pushl %%es		;"\
	"pushl %%ds		;"\
	"pushl %%ebp		;"\
	"pushl %%eax            ;"



#define RESTORE_REGS		\
	"pop %%eax		;"\
	"pop %%ebp              ;"\
	"pop %%ds		;"\
	"pop %%es		;"\
	"pop %%edi		;"\
	"pop %%esi		;"\
	"pop %%edx		;"\
	"pop %%ecx		;"\
	"pop %%ebx              ;"


union _i386_context {
  STACK_ELT regs[CONTEXT_REGS_NR];
  struct {
  STACK_ELT eax;
  STACK_ELT ebp;
  STACK_ELT ds;
  STACK_ELT es;
  STACK_ELT edi;
  STACK_ELT esi;
  STACK_ELT edx;
  STACK_ELT ecx;
  STACK_ELT ebx;
  }u;
}PACKED;
typedef union _i386_context i386_context;

#endif // ASSEMBLER

#endif // __SAVE_RESTORE
