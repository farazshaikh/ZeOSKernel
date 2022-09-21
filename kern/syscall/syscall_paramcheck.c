/** @file     syscall_paramcheck.c
 *  @brief    Takes care of routing systems call to and from User land to kernel.
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

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"


/** @function  tid_checker
 *  @brief     This function checks if the tid is a valid tid value or not
 *  @param     tid - thread id of a thread
 *  @return    KERN_SUCCESS on success; KERN err code
 */

KERN_RET_CODE tid_checker(int tid) {
  kthread       *thread;
  ktask         *pTask;
  KERN_RET_CODE ret = KERN_ERROR_GENERIC;
  FN_ENTRY();

  pTask = ((kthread *)tid)->pTask;

  Q_FOREACH( thread , &pTask->ktask_threads_head , kthread_next )
    if( thread == (kthread *)tid )
      ret = KERN_SUCCESS;

  FN_LEAVE();
  return ret;
}


/** @function  syscall_noargs_check
 *  @brief     This function is a generic syscall argument check template
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- fork() , gettid(), thread_fork(), get_ticks(), task_vanish(), vanish()  -- //

KERN_RET_CODE syscall_noargs_check(void *user_param_packet) {
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  // -- no action taken -- //

  FN_LEAVE();
  return ret;
}


/** @function  syscall_singleargs_check
 *  @brief     This function checks if the supplied single argument is valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- set_status(int status), sleep(int ticks) -- //

KERN_RET_CODE syscall_singleargs_check(void *user_param_packet) {
  KERN_RET_CODE ret = KERN_SUCCESS;
  int           get_int;
  FN_ENTRY();

  get_int = (int)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  get_int = get_int;
  // -- no other checks to be implemented here -- //

  FN_LEAVE();
  return ret;
}


#define VALID_COLOR_BITS 7

/** @function  syscall_settermcolor_check
 *  @brief     This function checks if the valid color is supplied to the console
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- set_term_color(int color) -- //

KERN_RET_CODE syscall_settermcolor_check(void *user_param_packet) {
  KERN_RET_CODE ret = KERN_SUCCESS;
  int           color;
  FN_ENTRY();

  color = (int)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  if( color >> VALID_COLOR_BITS ){
    DUMP("Failure: Parameter check failed for set_term_color syscall");
    return KERN_ERROR_INVALID_SYSCALL;
  }
  FN_LEAVE();
  return ret;
}


/** @function  syscall_readline_check
 *  @brief     This function checks if the arguments to readline are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- readline(int len, char *buf) -- //

KERN_RET_CODE syscall_readline_check(void *user_param_packet) {
  KERN_RET_CODE ret = KERN_SUCCESS;
  int           len;
  char          *buf;
  FN_ENTRY();

  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  // -- Check if the write buffer for given length is in process memory -- //
  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (void *)buf , len );
  if( KERN_SUCCESS != ret ) {
    DUMP("Failure: Parameter check failed for readline syscall");
    return KERN_ERROR_INVALID_SYSCALL;
  }
  FN_LEAVE();
  return ret;
}


/** @function  syscall_print_check
 *  @brief     This function checks if the arguments to print are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- print(int len, char *buf) -- //

KERN_RET_CODE syscall_print_check(void *user_param_packet) {
  int           len;
  char          *buf;
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  // -- Check if the read buffer for given length is in process memory -- //
  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (void *)buf , len );
  if( KERN_SUCCESS != ret ) {
    DUMP("Failure: Parameter check failed for print syscall");
    return KERN_ERROR_INVALID_SYSCALL;
  }
  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function  syscall_setcursorpos_check
 *  @brief     This function checks if the arguments to set_cursor_pos are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- set_cursor_pos(int row, int col) -- //

KERN_RET_CODE syscall_setcursorpos_check(void *user_param_packet) {
  //- function performs its own validatation
  return KERN_SUCCESS;
}


/** @function  syscall_getcursorpos_check
 *  @brief     This function checks if the arguments to get_cursor_pos are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- get_cursor_pos(int *row, int *col) -- //

KERN_RET_CODE syscall_getcursorpos_check(void *user_param_packet) {
  int           *rowp;
  int           *colp;
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  rowp  = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  colp  = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (char *)rowp , sizeof(int) );
  if( KERN_SUCCESS != ret ){
    DUMP("Failure: Parameter check failed for get_cursor_pos syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (char *)colp , sizeof(int) );
  if( KERN_SUCCESS != ret ){
    DUMP("Failure: Parameter check failed for get_cursor_pos syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  FN_LEAVE();
  return ret;
}


/** @function  syscall_cas2irunflag_check
 *  @brief     This function checks if the arguments to cas2i_runflag are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- cas2i_runflag(int tid, int *oldp, int ev1, int nv1, int ev2, int nv2) -- //

KERN_RET_CODE syscall_cas2i_check(void *user_param_packet) {
  int           tid;
  int           *oldp;
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  tid  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  oldp  = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (char *)oldp , sizeof(int) );
  if( KERN_SUCCESS != ret ){
    DUMP("Failure: Parameter check failed for cas2i_runflag syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  ret = tid_checker(tid);
  if( KERN_SUCCESS != ret ){
    DUMP("Failure: Parameter check failed for cas2i_runflag syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  FN_LEAVE();
  return ret;
}



/** @function  syscall_newpages_check
 *  @brief     This function checks if the arguments to newpages are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- newpages(void *base_addr, int len) -- //
#define PAGE_OFFSET_MASK 0xfff
#define PAGE_OFFSET( addr ) (addr)&PAGE_OFFSET_MASK

KERN_RET_CODE syscall_newpages_check(void *user_param_packet) {
  int           len;
  void          *base_addr;
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  base_addr  = (void *) (*(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0));
  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  // -- ensure that the base conditions are met -- //
  if( base_addr < (void *)USER_MEM_START ){
    DUMP("Failure: Parameter check failed for new_pages syscall");
    return KERN_PAGE_ERR;
  }

  if( PAGE_OFFSET((unsigned long) base_addr)){
    DUMP("Failure: Parameter check failed for new_pages syscall");
    return KERN_PAGE_ERR;
  }

  if( PAGE_OFFSET( len )) {
    DUMP("Failure: Parameter check failed for new_pages syscall");
    return KERN_PAGE_ERR;
  }

  FN_LEAVE();
  return ret;
}


/** @function  syscall_removepages_check
 *  @brief     This function checks if the arguments to remove_pages are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- removepages(void *base_addr) -- //

KERN_RET_CODE syscall_removepages_check(void *user_param_packet) {
  int           *base_addr;
  KERN_RET_CODE ret = KERN_SUCCESS;
  FN_ENTRY();

  base_addr  = (void *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (char *)base_addr , PAGE_SIZE );
  if( KERN_SUCCESS != ret ) {
    DUMP("Failure: Parameter check failed for remove_pages syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  FN_LEAVE();
  return ret;
}


#define NUMBER_OF_ARGS_LIMITS 8

/** @function  syscall_exec_check
 *  @brief     This function checks if the arguments to exec are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- exec(char *execname, char *args[]) -- //

KERN_RET_CODE syscall_exec_check(void *user_param_packet) {

  // -- The following check for exec parameters will not be required -- //
  // -- The way exec/loader is implemented, the parameters will be -- //
  // -- having valid address locations -- //
  return KERN_SUCCESS;
}


/** @function  syscall_ls_check
 *  @brief     This function checks if the arguments to ls are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- ls(int size,char *buf) -- //

KERN_RET_CODE syscall_ls_check(void *user_param_packet) {
  KERN_RET_CODE ret;
  int len;
  char *buf;

  FN_ENTRY();
  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , buf , len );
  if( KERN_SUCCESS != ret ) {
    DUMP("Failure: Parameter check failed for ls syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  return KERN_SUCCESS;
}


/** @function  syscall_wait_check
 *  @brief     This function checks if the arguments to wait are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

// -- Every call to below call(s) pass through the following function -- //
// -- wait(int *status) -- //

KERN_RET_CODE syscall_wait_check(void *user_param_packet) {
  int           *status;
  KERN_RET_CODE ret;
  FN_ENTRY();
  status  = (int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);

  ret = vmm_is_range_present( &((CURRENT_THREAD)->pTask->vm) , (char *)status , sizeof(int) );
  if( KERN_SUCCESS != ret ) {
    DUMP("Failure: Parameter check failed for wait syscall");
    return KERN_ERR_BAD_SYS_PARAM;
  }
  return KERN_SUCCESS;
}

// -- Every call to below call(s) pass through the following function -- //
// -- yield(int tid) -- //


/** @function  syscall_yield_check
 *  @brief     This function checks if the arguments to yield are valid
 *  @param     user_param_packet - address of parameter packet - %esi
 *  @return    KERN_SUCCESS on success; KERN err code
 */

KERN_RET_CODE syscall_yield_check(void *user_param_packet) {
  int tid;
  KERN_RET_CODE ret;
  FN_ENTRY();
  tid  = (int)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);

  if(tid == -1)
    return KERN_SUCCESS;

  ret = tid_checker(tid);
  return ret;
}
