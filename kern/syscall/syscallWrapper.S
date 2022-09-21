/** @file     syscallWrapper.S
 *  @brief    This file contains the system call wrappers
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <x86/seg.h>
#include <i386lib/i386saverestore.h>
	

.data				
				
.extern syscall_enter

never_land_syscall:
	xor %eax,%eax		#basically anything I used it to test pusha popa
	xor %ebx,%ebx
	xor %ecx,%ecx
	xor %edx,%edx

	xor %esi,%esi
	xor %edi,%edi
	xor %ebp,%ebp
	xor %eax,%eax

	pop %eax               #pop off 2 params
	pop %eax               #pop
	ret




#####################################################################
#
# IDT TABLE SETUP FOR SYSTEM CALLS 
# ................
# . GETTID_INT   <-------------------------------------+
# ................                                     |
# ................                                     |
# ................                                     |
# ................                                     |
# ................                                     |
# ................(IDTR)                               |
#                                                      |
#                                                      |
#  +===================                   ==========================+
#  |.....................                  .........................|
#  |sc_wrapper_template:.----(asmcode)--->.sc_wrapper_gettid       :|
#  |	      	        .   *malloced*    .                        .|
#  |   push 0xBABEBABE------*PATCHED*----->push "gettid_table_idx" .|
#  |   call sys_call_ent.                 .call sys_call_ent       .|
#  |   jmp sc_next_instr.         +=====<<.jmp sc_next_instruction .|
#  |.....................         |       ..........................|
#  |sc_next_instruction:.<<=======+       ==========================+
#  |offset...............                 (Thus every system call
#  |.....................                 Entry in IDT is malloced and
#  +====================                 patched)
#
#####################################################################

sc_wrapper_template:		#basic system call template wrapper
 	SAVE_REGS		#push all
 	movl $(SEGSEL_KERNEL_DS),%eax
 	movl %eax,%es		#set up the kernel data segment selector
 	movl %eax,%ds		#set up the kernel extra segment selector
	push %esi               #push syscall parameter block  
 	push $0xBABEBABE            #this will get patched to argument to be passed
 	                        #common system call handler
sc_patchoffset:
	leal  syscall_enter,%eax
 	call  %eax              #call the common system call handler
	add   $0x8,%esp
	mov   %eax,(%esp)           #save up the return value of this system call
	leal  sc_ret_from_syscall,%eax #absolute 32-bit address for the jump
 	jmp   *%eax		#NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
 	                        #THIS IS NOT A JuMp To ThE NeXt aDdReSs
 	                        #code block from sc_wrapper_template to
 	                        #to this jump instruction is MALLOCED
 	                        #to form multiple copies of it to be installed
 	                        #into the IDT table.
 	                        #each such malloced code block has its own eip
 	              
sc_ret_from_syscall:
 	RESTORE_REGS
	push %eax
	mov  $(SEGSEL_USER_DS),%eax
	movl %eax,%ds
	movl %eax,%es
	movl %eax,%gs
	movl %eax,%fs
	pop  %eax

 	iret
sc_end:

.global sc_wrapper_template	#address of the base ISR template
.global sc_patchoffset
.global sc_ret_from_syscall
.global sc_end






