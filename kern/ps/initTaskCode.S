/** @file     initTaskCode.S 
 *  @brief    This file contains helper function definitions 
 *            that are called by .c files that manage the task
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <x86/seg.h>

.data


.global get_esp
get_esp:	
	mov %esp,%eax
	add $4,%eax
	ret
	
.global get_ebp
get_ebp:	
	mov %ebp,%eax
	ret


/* Test loading a very simple binary in user mode */	
user_mode_init_code:
	int $0x42                #call exec esi is already loaded
	                         #with the initial binary
	jmp user_mode_init_code
user_mode_init_code_end:

.global user_mode_init_code
.global user_mode_init_code_end

