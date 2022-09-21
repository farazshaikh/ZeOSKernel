/** @file     syscall_console.c
 *  @brief    This file contains the system call handler for all console calls
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

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include "bootdrvlib/timer_driver.h"
#include "bootdrvlib/keyb_driver.h"


/** @function  syscall_getchar
 *  @brief     This function implements the get_char system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    character read from the read buffer queue
 */

KERN_RET_CODE syscall_getchar(void *user_param_packet) {
  KERN_RET_CODE ret;
  FN_ENTRY();

  ret = synchronous_readchar();

  FN_LEAVE();
  return ret;
}


/** @function  syscall_readline
 *  @brief     This function implements the read_line system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    returns the length of the buffer read
 */

KERN_RET_CODE syscall_readline(void *user_param_packet) {
  KERN_RET_CODE ret;
  int len;
  char *buf;
  FN_ENTRY();
  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);  
  memset(buf,0,len);
  ret = synchronous_readline(len,buf);
  FN_LEAVE();
  return ret;
}


/** @function  syscall_print
 *  @brief     This function implements the print system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE syscall_print(void *user_param_packet) {
  int           len;
  char         *buf;
  FN_ENTRY();

  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);

  putbytes(buf,len);
  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  syscall_settermcolor
 *  @brief     This function implements the set_term_color system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on successful completion
 */

KERN_RET_CODE syscall_settermcolor(void *user_param_packet) {
  KERN_RET_CODE ret;
  int           term_color;
  FN_ENTRY();

  term_color  = (int) GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  ret =  set_term_color(term_color);

  FN_LEAVE();
  return ret;
}


/** @function  syscall_setcursorpos
 *  @brief     This function implements the set_cursor_pos system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on successful completion
 */

KERN_RET_CODE syscall_setcursorpos(void *user_param_packet) {
  KERN_RET_CODE ret;
  int row,col;
  FN_ENTRY();

  row  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  col  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);
  ret  = set_cursor(row,col);

  FN_LEAVE();
  return ret;
}

/** @function  syscall_getcursorpos
 *  @brief     This function implements the get_cursor_pos system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE syscall_getcursorpos(void *user_param_packet) {
  int *rowp,*colp;
  FN_ENTRY();

  rowp  = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  colp  = *(int **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);
  get_cursor(rowp,colp);

  FN_LEAVE();
  return KERN_SUCCESS;
}
