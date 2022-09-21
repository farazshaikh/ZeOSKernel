/** @file     syscall_ls.c
 *  @brief    This file contains the system call handler for ls()
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
#include <elf_410.h>

#include <simics.h>
#include <asm.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "syscall_internal.h"
#include "i386lib/i386systemregs.h"
#include "bootdrvlib/timer_driver.h"
#include "bootdrvlib/keyb_driver.h"


/** @function  syscall_ls
 *  @brief     This function implements the ls system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    number of files in system on success; KERN err code on failure
 */

KERN_RET_CODE syscall_ls(void *user_param_packet) {
  int index;
  int n;
  int names_len;
  int len;
  char *buf;
  FN_ENTRY();
  len  = *(int *)GET_NTH_PARAM_FROM_PACKET(user_param_packet,0);
  buf  = *(char **)GET_NTH_PARAM_FROM_PACKET(user_param_packet,1);  


  // -- find the file names len -- //
  // -- => sum of lengths of all filenames in the ramdisk -- //
  index = 0;
  names_len = 0;
  while(index < exec2obj_userapp_count) {
    names_len += (strlen(exec2obj_userapp_TOC[index].execname) + 1);
    index++;
  }

  // -- if the supplied buffer is too small -- //
  if(names_len > len) { 
    return KERN_BUFFER_TOO_SMALL;
  }

  //- copy over filename -//
  memset(buf , 0 , len);
  index = 0;
  while(index < exec2obj_userapp_count) {
    n = strlen(exec2obj_userapp_TOC[index].execname);
    memcpy(buf,exec2obj_userapp_TOC[index].execname,n);
    buf += (n+1);
    index++;
  }

  FN_LEAVE();
  return index;
}
