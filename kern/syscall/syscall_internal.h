/** @file     syscall_internal.h
 *  @brief    This file contains the interfaces for the handling system call handlers
 *
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _SYS_CALL_INTRNL_H
#define _SYS_CALL_INTRNL_H
#include <kern_common.h>

#define GET_NTH_PARAM_FROM_PACKET(user_param_packet,n)	\
  ((char *)(   (char *)(user_param_packet) + sizeof(uint32_t) *(n) ))


KERN_RET_CODE syscall_exec(void *user_param_packet); 
KERN_RET_CODE syscall_gettid(void *user_param_packet);
KERN_RET_CODE syscall_set_status(void *user_param_packet);
KERN_RET_CODE syscall_vanish(void *user_param_packet);
KERN_RET_CODE syscall_taskvanish(void *user_param_packet);


//-- Console syscalls --//
KERN_RET_CODE syscall_getchar(void *user_param_packet);
KERN_RET_CODE syscall_readline(void *user_param_packet);
KERN_RET_CODE syscall_print(void *user_param_packet);
KERN_RET_CODE syscall_settermcolor(void *user_param_packet);
KERN_RET_CODE syscall_setcursorpos(void *user_param_packet);
KERN_RET_CODE syscall_getcursorpos(void *user_param_packet);

KERN_RET_CODE syscall_fork(void *user_param_packet); 
KERN_RET_CODE syscall_getticks(void *user_param_packet);
KERN_RET_CODE syscall_sleep(void *user_param_packet);
KERN_RET_CODE syscall_wait(void *user_param_packet);
KERN_RET_CODE syscall_yield(void *user_param_packet);
KERN_RET_CODE syscall_threadfork(void *user_param_packet);


KERN_RET_CODE syscall_ls(void *user_param_packet);
KERN_RET_CODE syscall_halt(void *user_param_packet);

KERN_RET_CODE syscall_newpages(void *user_param_packet);
KERN_RET_CODE syscall_removepages(void *user_param_packet);
KERN_RET_CODE syscall_cas2irunflag(void *user_param_packet);


/*Exported Function Prototypes*/
void thread_setup_ret_from_fork(kthread *thread) ;

// -- Checker function prototypes -- //
KERN_RET_CODE syscall_noargs_check(void *user_param_packet);
KERN_RET_CODE syscall_singleargs_check(void *user_param_packet);
KERN_RET_CODE syscall_settermcolor_check(void *user_param_packet);
KERN_RET_CODE syscall_readline_check(void *user_param_packet);
KERN_RET_CODE syscall_print_check(void *user_param_packet);
KERN_RET_CODE syscall_setcursorpos_check(void *user_param_packet);
KERN_RET_CODE syscall_getcursorpos_check(void *user_param_packet);
KERN_RET_CODE syscall_cas2i_check(void *user_param_packet);
KERN_RET_CODE syscall_newpages_check(void *user_param_packet);
KERN_RET_CODE syscall_removepages_check(void *user_param_packet);
KERN_RET_CODE syscall_exec_check(void *user_param_packet);
KERN_RET_CODE syscall_ls_check(void *user_param_packet);
KERN_RET_CODE syscall_wait_check(void *user_param_packet);
KERN_RET_CODE syscall_yield_check(void *user_param_packet);

#endif // _SYS_CALL_INTRNL_H
