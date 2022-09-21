/** @file     syscall_exec.c
 *  @brief    This file contains the system call handler for exec()
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


#define GET_ARG_PTR(pppc,idx) *(((*pppc))+(idx)) 

struct  _exec_args {
  int argc;
  int data_len;
  char *filename;
  char *argv[0];
}PACKED;
typedef struct _exec_args exec_args;


// -- Below 2 functions are used because of the way our loader is implemented -- //
// -- Our loader goes on to spawn a new task and loads the file there -- //
// -- and then loads the file on that task -- //
// -- But doing this impiles that we lose internal pointers to argv string -- //
// -- that are setup in exec_args.  -- //
// -- We therefore copy over the arguments to exec onto the stack -- //

/** @function  exec_copy_argv_to_stack
 *  @brief     This function copies the arguments over to a new stack
 *  @param     stack     - pointer to the source stack
 *  @param     newStack  - pointer to placeholder of dest stack
 *  @param     exec_args - pointer to arguments to exec
 *  @return    void
 */

void exec_copy_argv_to_stack(char *stack,
			     char **newStack,
			     exec_args *exec_args) 
{
  char *data_start;
  char *copy_at;
  int  i;
  
  data_start = stack;
  data_start = data_start - sizeof(STACK_ELT);
  data_start = data_start - exec_args->data_len;
  data_start = (char *)((unsigned long)data_start & (unsigned long)(~0x3));
  memset(data_start,0,(unsigned long)stack - (unsigned long)data_start);

  
  // Start copying stuff //
  copy_at = data_start;
  for(i=0;i<exec_args->argc;i++) {
    char *temp = copy_at;
    memcpy(copy_at,exec_args->argv[i],strlen(exec_args->argv[i]));
    copy_at += strlen(exec_args->argv[i]);
    copy_at++;
    
    // overwrite address now //
    exec_args->argv[i] = temp;
  }

  // copy out argv array onto the stack //
  for(i=exec_args->argc-1;
      i>=0;
      i--)
  {
    data_start -= (sizeof(STACK_ELT));
    *(char **)(data_start) = exec_args->argv[i];
  }


  //-- copy out argv --//
  data_start -= (sizeof(STACK_ELT));
  *(char **)(data_start) = data_start + sizeof(STACK_ELT);


  //-- copy out argc --//
  data_start -= (sizeof(STACK_ELT));
  *(int *)(data_start) = exec_args->argc;


  //-- copy an dummy return address --//
  data_start -= (sizeof(STACK_ELT));
  *(int *)(data_start) = 0xDEADBEEF;


  *newStack = data_start;
  return;
}


/** @function  exec_copy_argv
 *  @brief     This function copies the argument string
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @param     exec_args         - pointer to placeholder of arguments to exec
 *  @return    void
 */

KERN_RET_CODE exec_copy_argv(void *user_param_packet,
			     exec_args **exec_args)
{
  struct _exec_args *local_exec_args=NULL; 
  char     **filename=NULL;
  char    ***argv=NULL;
  char      *data=NULL;
  int        data_len=0;
  int        argc=0;
  int        i;

  //-- get out the two parameters --//
  filename = (char **)user_param_packet; 
  argv = (char ***)((char *)user_param_packet + sizeof(STACK_ELT));

  if(!filename || !argv) {
    DUMP("NULL parameters to sys_exec");
    return KERN_ERROR_GENERIC; 
  }
  
  for(argc=0 ; 
      GET_ARG_PTR(argv,argc) && GET_ARG_PTR(argv,argc)[0] != '\0' ; 
      argc++) 
  { 
    data_len += strlen(GET_ARG_PTR(argv,argc)) + 1;
  } 
  data_len++;
  // argc++; //6 for foo a b c d [NULL]
  // argc now contains elements in argv array +
  data_len += strlen(*filename) + 1;

  local_exec_args = malloc(sizeof(*local_exec_args) + 
			   (sizeof(char *) * argc)+
			   data_len);
  memset(local_exec_args,0,sizeof(*local_exec_args) + 
			   (sizeof(char *) * argc)+data_len);

  if(!local_exec_args)
    return KERN_NO_MEM; 

  local_exec_args->argc = argc;
  local_exec_args->data_len = data_len;
  data = (char *) (&local_exec_args->argv[argc]);

  // copy stuff over //
  for(i=0;i<argc;i++) {
    memcpy(data,
	   GET_ARG_PTR(argv,i),
	   strlen(GET_ARG_PTR(argv,i))
	   );
    local_exec_args->argv[i] = data;
    data =  data + strlen(GET_ARG_PTR(argv,i)) + 1;
  }
    
  local_exec_args->filename = data;
  memcpy(local_exec_args->filename,*filename,strlen(*filename));
  data =  data + strlen(*filename) + 1;


  //-- We cannot live with silent corruptions -//
  assert(data <= (char *)local_exec_args + 
	 sizeof(*local_exec_args) + 
	 sizeof(char *) * argc +
	 data_len);
  

  *exec_args=local_exec_args;
  return KERN_SUCCESS;
}


/** @function  syscall_exec
 *  @brief     This function implements the exec system call
 *  @param     user_param_packet - %esi as passed down from user mode
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE syscall_exec(void *user_param_packet) { 
  exec_args *local_exec_args; 
  KERN_RET_CODE ret;
  unsigned long start_address;
  unsigned long u_stack,new_u_stack;
  FN_ENTRY();


  task_fork_lock((CURRENT_THREAD)->pTask);    


  //-- Get the filename out --//
  ret = exec_copy_argv(user_param_packet,&local_exec_args); 
  if( KERN_SUCCESS != ret ) {
    task_fork_unlock((CURRENT_THREAD)->pTask);    
    return ret;
  }
  DUMP("syscall_exec params %s",local_exec_args->filename);    

  //-- load the filename --//
  ret =  load_elf(CURRENT_THREAD->pTask,
		  local_exec_args->filename,
		  &start_address,
		  &u_stack);

  if(ret != KERN_SUCCESS) { 
    DUMP("load_elf failed kill process");
    free(local_exec_args);
    task_fork_unlock(CURRENT_THREAD->pTask);    
    return ret;
  }

  //-- Setup the argv stack --//
  exec_copy_argv_to_stack((char  *)  u_stack,
			  (char **) &new_u_stack,
			  local_exec_args);
  
  //-- setup the iret frame --//
  thread_setup_iret_frame(CURRENT_THREAD,
			  (STACK_ELT)  new_u_stack,
			  (STACK_ELT)  start_address,
			  (STACK_ELT)  0
			  ); 
  free(local_exec_args);

  task_fork_unlock(CURRENT_THREAD->pTask);    
  FN_LEAVE();
  return KERN_SUCCESS;
}
