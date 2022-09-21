/** @file     i386isrwrapper.S 
 *  @brief    This file contains the implementation of ISR wrapper
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#include <x86/seg.h>
#include <i386lib/i386saverestore.h>
	
	                        # Code is defined in RW .data section 4 patching
.data				# Was done for testing code in Userland
				# Code written in .data/.bss section is RWritable

never_land_isr:
	xor %eax,%eax		#basically anything I used it to test pusha popa
	xor %ebx,%ebx
	xor %ecx,%ecx
	xor %edx,%edx
	
	xor %esi,%esi
	xor %edi,%edi
	xor %ebp,%ebp
	xor %eax,%eax
	ret
	
		
isr_wrapper_template:		#basic ISR template wrapper
	SAVE_REGS		#push all
	push %esi               #has to match syscall enter's sytle
	push $0xDEADBEEF        #has to match syscall enter's style
iw_patchoffset:	
	call never_land_isr	#patch isr template here to actual C ISR handler
iw_next_instroffset:	
	add   $0x8,%esp	        #pop off esi and deadbeaf >>*system call idx*<<	
	RESTORE_REGS
	iret
iw_end:

.global isr_wrapper_template	#address of the base ISR template
.global iw_patchoffset
.global iw_next_instroffset
.global iw_end
	
