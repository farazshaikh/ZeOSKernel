/** @file     syscall.c
 *  @brief    This file contains syscall jumptable definition
 *            and has routines that take care of userland to kernel
 *            and kernel to userland jumps
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


/** @typedef   GENERIC_FUNCTION_CALL_ADDRESS
 *  @brief     generic function call address type for the system call handler
 */

typedef KERN_RET_CODE (*GENERIC_FN_CALL_ADDRESS)(void *user_param_packet);



/** @typedef   SYS_CALL
 *  @brief     Single element of the syscall table describing sigle system call
 */

typedef struct _SYS_CALL {
  int   int_nr;
  GENERIC_FN_CALL_ADDRESS fn_address;
  int   params_nr;
  GENERIC_FN_CALL_ADDRESS fn_address_param_check;
}SYS_CALL;


KERN_RET_CODE syscall_unimpl(void *user_param_packet);


/** @macro    TOTAL_SYSTEM_CALLS
 *  @brief    total system calls supported by the kernel
 */

#define TOTAL_SYSTEM_CALLS (CAS2I_RUNFLAG_INT - SYSCALL_INT + 1)


/** @global   sys_call_table
 *  @brief    dispatch table for system calls
 */
SYS_CALL sys_call_table[TOTAL_SYSTEM_CALLS] =
  {
    { SYSCALL_INT         , syscall_unimpl,       0 , syscall_unimpl },
    { FORK_INT            , syscall_fork,         0 , syscall_noargs_check},
    { EXEC_INT            , syscall_exec,         0 , syscall_exec_check},

    { WAIT_INT            , syscall_wait,         0 , syscall_wait_check},
    { YIELD_INT           , syscall_yield,        0 , syscall_yield_check},
    { GETTID_INT          , syscall_gettid,       0 , syscall_noargs_check},
    { NEW_PAGES_INT       , syscall_newpages,     0 , syscall_newpages_check},
    { REMOVE_PAGES_INT    , syscall_removepages,  0 , syscall_removepages_check},
    { SLEEP_INT           , syscall_sleep,        0 , syscall_singleargs_check},
    { GETCHAR_INT         , syscall_getchar,      0 , syscall_noargs_check},
    { READLINE_INT        , syscall_readline,     0 , syscall_readline_check},
    { PRINT_INT           , syscall_print,        0 , syscall_print_check},
    { SET_TERM_COLOR_INT  , syscall_settermcolor, 0 , syscall_settermcolor_check},
    { SET_CURSOR_POS_INT  , syscall_setcursorpos, 0 , syscall_setcursorpos_check},
    { GET_CURSOR_POS_INT  , syscall_getcursorpos, 0 , syscall_getcursorpos_check},
    { THREAD_FORK_INT     , syscall_threadfork,   0 , syscall_noargs_check},
    { GET_TICKS_INT       , syscall_getticks,     0 , syscall_noargs_check},
    { MISBEHAVE_INT       , syscall_unimpl,       0 , syscall_unimpl},
    { HALT_INT            , syscall_halt,         0 , syscall_noargs_check},
    { LS_INT              , syscall_ls,           0 , syscall_ls_check},
    { TASK_VANISH_INT     , syscall_taskvanish,   0 , syscall_noargs_check},
    { SET_STATUS_INT      , syscall_set_status,   0 , syscall_singleargs_check},
    { VANISH_INT          , syscall_vanish,       0 , syscall_noargs_check},
    { CAS2I_RUNFLAG_INT   , syscall_cas2irunflag, 0 , syscall_cas2i_check}
  };


/** @macro   IS_VALID_SYSTEM_CALL_IDX
 *  @brief   bound checking for the sys_call_table
 */

#define IS_VALID_SYSTEM_CALL_IDX(system_call_idx)			\
  (system_call_idx >= 0 && system_call_idx < TOTAL_SYSTEM_CALLS)


/** @global   sc_wrapper_template sc_patchoffset sc_end sc_next_instroffset
 *  @brief    exports from .asm wrapper to assist code patching
 */

extern char sc_wrapper_template;
extern char sc_patchoffset;
extern char sc_end;
extern char sc_ret_from_syscall;


/** @global   psc_wrapper_template psc_patchoffset psc_end psc_ret_from_syscall
 *  @brief    temporary variables for readability
 */

char *psc_wrapper_template;            //- Don't need so many globals   -//
char *psc_patchoffset;                  //- But Helps readability        -//
char *psc_end;
char *psc_ret_from_syscall;

int   sc_patch_offset;


/** @function  offsets_fixup_sc
 *  @brief     This function fixes up offsets required for
 *             code patching in the ISR template
 *  @note      three parameter address (externs from ASM file) are used
 *             declaring the system call template
 *             [sc_wrapper_template/sc_end/sc_patchoffset/sc_ret_from_syscall]
 *  @return    KERN_SUCCESS on completion
 */

static KERN_RET_CODE offsets_fixup_sc(void)
{
  FN_ENTRY();

  //-- We defined code in asm .data section --//
  psc_wrapper_template = &sc_wrapper_template;
  psc_patchoffset = &sc_patchoffset;
  psc_end         = &sc_end;
  psc_ret_from_syscall = &sc_ret_from_syscall;

  //-- Get absolute values for patch offsets -//
  sc_patch_offset     = psc_patchoffset - psc_wrapper_template;
  FN_LEAVE();
  return KERN_SUCCESS;
}


#define SC_PATCH_MAGIC 0xBABEBABE
#define PUSH_IMMIDIATE_OPCODE 0x68

/** @function  patch_sc_code_block
 *  @brief     This function patches a system call template code block with
 *             with appropriate parameter pushing
 *  @param     code_block   - address of the code block to be patched
 *  @param     sys_call_idx - to be passed to the system call invocation
 *  @return    KERN_SUCCESS after completion
 */

static KERN_RET_CODE patch_sc_code_block(
				      char *code_block,
				      int sys_call_idx)
{
  FN_ENTRY();
  //-- offsets_fixup base ISR wrapper template --//
  offsets_fixup_sc();

  //-- get the offsets of the PUSH 0xBABEBABE instruction --//
  assert( SC_PATCH_MAGIC == *(int*) (code_block + sc_patch_offset - sizeof(int)) );

  //- patch up "push 0xBABEBABE" to "push sys_call_idx" -//
  *(int*)(code_block + sc_patch_offset - sizeof(int)) = sys_call_idx;

  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function  i386_sc_set_idt_entry
 *  @brief     This function sets up the IDT entry for a system call
 *             with appropriate parameter pushing
 *  @param     sys_call_idx   - system call number to be passed to sys call hander
 *  @param     int_nr         - the interrupt number for the system call
 *  @param     pSyscall_enter - the system call C handler
 *  @return    KERN_SUCCESS on success; KERN err codes on failure
 */

KERN_RET_CODE i386_sc_set_idt_entry(int sys_call_idx,
				    int int_nr,
				    KERN_RET_CODE ASM_LINKAGE (*pSyscall_enter)(int,void *))
{
  KERN_RET_CODE ret;
  char *pSCWrapperCode = NULL; //-- Will be generated by malloc & code copy -//
  FN_ENTRY();
  DEBUG_PRINT("Installing 0x%x SYSTEM_CALL at IDT 0x%x",sys_call_idx,int_nr);

  //-- fixup the offsets upfront --//
  offsets_fixup_sc();

  //-- Allocate Code block for the new system_call_wrapper --//
  pSCWrapperCode = malloc( psc_ret_from_syscall - psc_wrapper_template );
  if( NULL == pSCWrapperCode ) {
    DUMP( "cannot allocate memory for installing system call wrapper" );
    return KERN_NO_MEM;
  }


  //- copy off the system call template code into allocated code block -//
  memcpy( pSCWrapperCode ,
	  psc_wrapper_template ,
	  psc_ret_from_syscall - psc_wrapper_template );


  //- patch the syscall idx into the code -//
  //- a.k.a Plant Bubble gum prince in choclate land -//
  ret = patch_sc_code_block( pSCWrapperCode ,sys_call_idx );
  assert( KERN_SUCCESS == ret );


  //-- Install system call entry into IDT now --//
  ret = i386_set_idt_entry((char *) idt_base(),
			   SEGSEL_KERNEL_CS,
			   pSCWrapperCode,
			   int_nr,
			   i386_GATE_TYPE_TRAP,
			   i386_PL3
			   );
  assert( KERN_SUCCESS == ret );
  DEBUG_PRINT("bubble gum prince glued in choclate land");
  FN_LEAVE();
  return ret;
}


/** @function  syscall_unimpl
 *  @brief     system call handling function used when calls aren't handled
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_ERROR_UNIMPLEMENTED;
 **/

KERN_RET_CODE syscall_unimpl(void *user_param_packet) {
  return KERN_ERROR_UNIMPLEMENTED;
}


/** @function  syscall_entry
 *  @brief     common function for all system calls
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @param     system_call_idx   - index of the system call in the system call table
 *  @return    return code from the called handler function
 *
 *  @NOTE: THE SYSCALL INTERFACE IS NOT CHANGED AND IS SAME AS PEBBLES SPEC
 *         SYSCALL IDENTIFICATION
 *         This is the C function called from the installed IDT entry
 *         The installed ISR code block is capable of pushing 2 param before the
 *         call to syscall_enter() function. In case of system calls the ISR
 *         code block pushes %esi and the index of system_call in the
 *         sys_call_table.
 *
 */

KERN_RET_CODE ASM_LINKAGE syscall_enter(int system_call_idx,void *user_param_packet) {
  KERN_RET_CODE ret;
  DEBUG_PRINT("system call %d called",system_call_idx);

  if(!IS_VALID_SYSTEM_CALL_IDX(system_call_idx)) {
    return KERN_ERROR_INVALID_SYSCALL;
  }

  /* Check the user parameter block */
  ret = sys_call_table[system_call_idx].fn_address_param_check(user_param_packet);
  if(ret != KERN_SUCCESS)
    return ret;


  /* call the actual system call handler */
  return sys_call_table[system_call_idx].fn_address(user_param_packet);
}



/** @function  syscall_init
 *  @brief     sets up the IDT entry for a all system calls
 *             with appropriate parameter pushing
 *  @param     none
 *  @return    KERN_SUCCESS on success; KERN err codes on failure
 */

KERN_RET_CODE syscall_init(void) {
  int i;
  KERN_RET_CODE ret;
  FN_ENTRY();

  /* install all the system call handlers using our hot-patched IDT installer */
  for(i=0; i < TOTAL_SYSTEM_CALLS ; i++)  {
    ret = i386_sc_set_idt_entry(i,sys_call_table[i].int_nr,syscall_enter);
    if(KERN_SUCCESS != ret) {
      FN_LEAVE();
      return ret;
    }
  }
  FN_LEAVE();
  return KERN_SUCCESS;
}
