/** @file     task.c 
 *  @brief    This file contains the definitions of task handling functions,
 *            functions that initialize a task and functions that setup 
 *            the task stack frame for mode and context switches
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <x86/seg.h>
#include <x86/cr.h>
#include <malloc.h>

#include <simics.h>
#include <asm.h>
#include <cr.h>
#include <eflags.h>  

#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "i386lib/i386systemregs.h"
#include "i386lib/i386saverestore.h"
#include "bootdrvlib/timer_driver.h"

ktask *idle_task;        //- twidler task calling schedule in a loop
                         //- for scheduling runnable process 

ktask *init_task;        //- first User mode task. Initial binary image
                         //- is hand crafted to do an exec system call 
                         //- with a specified file name 

extern char sc_ret_from_syscall;

/** @function  PAGING_ENABLE
 *  @brief     This function enables paging globally
 *  @param     none
 *  @return    void
 */

void PAGING_ENABLE() {
    uint32_t cr0;	
			
    cr0 = get_cr0();	
    cr0 |= CR0_PG;	
    set_cr0(cr0);	
			
    DUMP("Paging enabled");
  }

/** @function  is_idle_thread
 *  @brief     This function checks if the current thread is the IDLE thread
 *             and returns the boolean value for that check
 *  @param     none
 *  @return    1 - if the current thread is IDLE thread; 0 otherwise
 */

int is_idle_thread() { 
  //-- Initial thread of the idle task --//
  return ( CURRENT_THREAD == (kthread *) idle_task ); 
}

/** @function  get_idle_thread
 *  @brief     This function uses the global pointer to idle task
 *             and returns the pointer to the IDLE thread in the kernel
 *  @param     none
 *  @return    pointer to the IDLE thread
 */

kthread * get_idle_thread() { 
  return &idle_task->initial_thread;
}

// -- function prototype (defined below) -- //
KERN_RET_CODE setup_init_code( ktask *init_task );

/** @function  thread_stack_push
 *  @brief     This function pushes the supplied value into the kernel stack
 *             and appropriately sets the kernel stack pointer
 *  @param     thread - pointer to the thread whose kernel stack is used in push
 *  @param     val    - 32-bit value that is pushed
 *  @return    void
 */

void thread_stack_push(kthread  *thread,
		       STACK_ELT val) { 
  *(--thread->context.r_esp) = val;
}

/** @function  thread_setup_iret_frame
 *  @brief     This function sets up the iret frame that will be used
 *             when the task switches from kernel mode to user mode
 *  @param     thread     - pointer to the thread whose kernel stack is used in push
 *  @param     user_stack - pointer to the top of user_stack
 *  @param     retIP      - pointer to the stack of code/execution in userland
 *  @param     error_code - error_code value that is passed to the user mode
 *  @return    void
 */

void thread_setup_iret_frame(kthread   *thread,
			   STACK_ELT  user_stack,
			   STACK_ELT  retIP,
			   STACK_ELT  error_code
			   ) 
{ 
  IRET_FRAME *pIretFrame;
  uint32_t    eflags; 
  FN_ENTRY();

  //-- setup the iret frame -//
  pIretFrame            =  ((IRET_FRAME *) thread->context.kstack) - 1;

  //- setup the eflags correctly -//
  eflags = get_eflags();
  eflags &= ~EFL_IOPL_RING3;
  eflags &= ~EFL_AC; 
  eflags |=  EFL_IF;
  eflags |=  0x2;
  

  //-- Setup the kernel iret frame -//
  // pIretFrame->error_code = (STACK_ELT) (error_code);// (0_code);
  pIretFrame->eip        = (STACK_ELT) (retIP);
  pIretFrame->cs         = (STACK_ELT) SEGSEL_USER_CS;
  pIretFrame->eflags     = (STACK_ELT) eflags;
  pIretFrame->esp        = (STACK_ELT) user_stack;
  pIretFrame->ss         = (STACK_ELT) SEGSEL_USER_DS; 
  FN_LEAVE();
}

/** @function  thread_setup_ret_from_syscall
 *  @brief     This function sets up the iret frame, the syscall return sections
 *             area of stack to hold the usermode registers, and register context
 *             of the current thread during a context switch
 *  @param     thread     - pointer to the thread whose kernel stack is used in push
 *  @param     user_stack - pointer to the top of user_stack
 *  @param     retIP      - pointer to the stack of code/execution in userland
 *  @param     error_code - error_code value that is passed to the user mode
 *  @param     sysenter_user_context  - usermode register context
 *  @param     context_switch_context - context switch register context
 *  @return    void
 */

void thread_setup_ret_from_syscall(
				 kthread      *thread,
				 STACK_ELT     user_stack,
				 STACK_ELT     retIP,
				 STACK_ELT     error_code,
				 i386_context *sysenter_user_context,
				 i386_context *context_switch_context
				 ) 
{
  int i;
  //-- setup the iret frame --//
  thread_setup_iret_frame(thread,
			user_stack,
			retIP,
			error_code);
  thread->context.r_esp -= sizeof(IRET_FRAME) / sizeof(STACK_ELT);

  //-- setup umode registers                                      --//
  for( i=CONTEXT_REGS_NR - 1 ; i >= 0 ; i--)
    thread_stack_push(thread,sysenter_user_context->regs[i]);

  //-- save the return address to ret_from_system call            --//
  thread_stack_push(thread,(STACK_ELT) &sc_ret_from_syscall);

  //-- context switch pushes these regs to avoid clobering         -//
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB1); //-ebx
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB2); //-esi
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB3); //-edi
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB4); //-ebp

  thread_stack_push(thread,(STACK_ELT) 0xBABABAB5); //-add    $0xc,%esp
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB6); 
  thread_stack_push(thread,(STACK_ELT) 0xBABABAB7); 

  //-- save registers poped by context switch                     --//
  for(i=CONTEXT_REGS_NR - 1 ; i >= 0 ; i--)
    thread_stack_push(thread,context_switch_context->regs[i]);

  return;
}


#define FOR_EVER 1
int     retard; // -- retard == slow down (not to be confused) -- //

/** @function  task_run_idle_loop
 *  @brief     This function sets up the idle task and IDLE thread
 *             The IDLE thread just spins FOR_EVER and 
 *             (future scope) can be used for system diagnostic purposes
 *  @param     idle_task - pointer to idle task
 *  @return    This function doesn't return
 */

  //-- Q) Why do we have an idle task ? --//
  /*
    Glossary: 
    CURRENT complaint Kernel stack:
    A kernel stack which has the current task stacked up at the lowest
    address. On such stacks "current" macro yields the current task.

    A. REST of the implmentation from here 
    works with ktask structs and ALL functions depend 
    on the availability of "current" macro which gets the current 
    task struct based on the %esp value. 

    Not having an initial stack that supports use of CURRENT introduces 
    special case to be handled for the first task (just 410 boot off)
    So we owe up our current 410 stack and setup a new stack which 
    is CURRENT complaint i.e with the ktask struct on the end of stack. 
   */

KERN_RET_CODE task_run_idle_loop(ktask *idle_task) {
  KERN_RET_CODE ret;
  
  idle_task->initial_thread.context.r_esp[-3] = get_esp();
  idle_task->initial_thread.context.r_esp[-4] = get_ebp();
  idle_task->initial_thread.context.r_esp    -= 4;

  //-- Enable paging --//
  set_cr3((uint32_t)idle_task->vm.pde_base);
  PAGING_ENABLE();

  ret = timer_set_callback(scheduler_timer_callback);
  if( KERN_SUCCESS != ret ) { 
    DUMP("timer_set_callback failed with error code %x" , ret);
    return ret;
  }

  //- Switch to "current" compliant stack --//
  __asm__ (
	   "xor %%ebp,%%ebp;"
	   "mov %0,%%esp;"
	   :
	   :"m" (idle_task->initial_thread.context.r_esp)
	   );
  //-- don't refer to anything on the idle thread's stack --//

  // -- Global Interrupt Enable to enable scheduling and clock ticks -- //
  enable_interrupts();

  while( FOR_EVER ) {
    if(retard++ == 100000) {
      DUMP("Idle thread:");
    }
    //-- idle is always runnable --//
    schedule(CURRENT_RUNNABLE);
  }

  //-- Never should come here --//
  panic("Kernel entered pre task struct programming mode");
  return KERN_ERROR_UNIMPLEMENTED;
}


//- DMZ -//
extern char user_mode_init_code;
extern char user_mode_init_code_end;
char *puser_mode_init_code;
char *puser_mode_init_code_end;

/** @function  task_setup_init_code
 *  @brief     This function sets up the init task and INIT thread
 *             The INIT binary is coded (#defined) in task.h include,
 *             and the below function loads, maps VM, initializes and runs init.
 *  @param     init_task - pointer to init task
 *  @param     initial_binary - pointer to the filename of the initial binary
 *  @return    KERN_SUCCESS on successful initialization
 */

KERN_RET_CODE task_setup_init_code(
				   ktask *init_task,
				   char  *initial_binary
				   )
{
  i386_context u_context,switch_context;
  unsigned int init_code_section_len;
  char         *binary_name_address;

  PTE *new_pte;
  KERN_RET_CODE ret;
  PFN user_mode_pfn;
  PDE *pde_base;
  int i; 

  //-- pointer fixup --//
  puser_mode_init_code_end = &user_mode_init_code_end;
  puser_mode_init_code     = &user_mode_init_code;


  //-- Allocate an user mode PTE page -//
  new_pte = smemalign( PAGE_SIZE , PAGE_SIZE );
  memset( new_pte , 0 , PAGE_SIZE );
  
  //-- Allocate a page for init user mode code --//
  ret = vmm_get_free_user_pages(&user_mode_pfn);
  assert(ret == KERN_SUCCESS);

  //-- map in init user mode code/data/stack page --//
  new_pte[0].PRESENT         = 1;
  new_pte[0].RW              = 1;
  new_pte[0].US              = 1;
  new_pte[0].GLOBAL          = 0;

  new_pte[0].ADDRESS         = user_mode_pfn;

  //-- get free PDE entry --//
  pde_base = init_task->vm.pde_base;
  for(i=0;i<PTE_PER_PAGE;i++) {
    if(!pde_base[i].PRESENT)
      break;
  }
  
  //-- Install the PTE into PDE --//
  pde_base[i].PRESENT        = 1;
  pde_base[i].RW             = 1;
  pde_base[i].US             = 1;
  pde_base[i].GLOBAL         = 0;
  pde_base[i].ADDRESS        = (unsigned long)new_pte >> PAGING_PAGE_OFFSET_BITS;

  //-- init task user mode code setup --//
  //-- init process has on 1 user mode page (0^) --//
  /*
    +-----------------       page end
    >.stack %esp                |
    >                           |   
    >                       <one page>
    {.heap "init_bin_name" %esi |
    +.code_end                  |
    +  execv(init_bin_name)     |
    +.code_start                |
    +-----------------  page start (USER_MEM_START) 
  */

  memset((char *)(user_mode_pfn << PAGING_PAGE_OFFSET_BITS) ,
	 0 , 
	 PAGE_SIZE );

  //-- copy binary name on top of stack --//
  binary_name_address = (char *)(user_mode_pfn << PAGING_PAGE_OFFSET_BITS) + PAGE_SIZE - 1;
  binary_name_address = binary_name_address - strlen(initial_binary) - 1;
  memcpy(binary_name_address,initial_binary,strlen(initial_binary));

  //-- copy in init code         --//
  init_code_section_len = puser_mode_init_code_end - puser_mode_init_code;
  memcpy((char *)(user_mode_pfn << PAGING_PAGE_OFFSET_BITS) ,
	 puser_mode_init_code , 
	 init_code_section_len);


  //-- make esi packet --//
  STACK_ELT *esi_packet = (STACK_ELT *)((char *)(user_mode_pfn << PAGING_PAGE_OFFSET_BITS)
			   + init_code_section_len);
  esi_packet[0] = (STACK_ELT) USER_MEM_START + PAGE_SIZE - strlen(initial_binary) - 2;
  esi_packet[1] = (STACK_ELT) USER_MEM_START + init_code_section_len + (sizeof(STACK_ELT) * 2);
  esi_packet[2] = (STACK_ELT) USER_MEM_START + PAGE_SIZE - 1;

  //-- build up kernel mode stack as expected by --//
  //-- ret_from_syscall                          --//
  //-- the task starts in schedule()             --//
  assert(init_task->initial_thread.context.kstack == 
	 init_task->initial_thread.context.r_esp);

  //-- setup the ret from sys call frame --//
  memset(&u_context,0xcc,sizeof(switch_context));
  memset(&switch_context,0xcc,sizeof(switch_context));

  u_context.u.esi = USER_MEM_START + init_code_section_len;
  u_context.u.ebp = 0x0;
  u_context.u.es  = SEGSEL_USER_DS;
  u_context.u.ds  = SEGSEL_USER_DS;

  switch_context.u.es = SEGSEL_KERNEL_DS;
  switch_context.u.ds = SEGSEL_KERNEL_DS;

  thread_setup_ret_from_syscall(&init_task->initial_thread,
				(USER_MEM_START + PAGE_SIZE - 2 - strlen(initial_binary) - 8) & ~0x3,        // esp
			        USER_MEM_START,             // eip
				0,                          // error code
				&u_context,
				&switch_context             // all start here
				);
  return KERN_SUCCESS;
}


/** @function  task_init
 *  @brief     This function is called by the base scheduler to handcraft
 *             the first task (init) in the kernel
 *             the init task is hand-crafted and the function 
 *             waits FOR_EVER at idle task creation
 *  @param     initial_binary - pointer to the filename of the initial binary
 *  @return    on successful initialization/functioning, this function never returns
 */

KERN_RET_CODE task_init(char *initial_binary) { 
  int ret;
  FN_ENTRY();


  //- Create the idle task -//
  ret = vmm_init_task_vm( NULL, &idle_task );
  if( ret != KERN_SUCCESS )  {
    DUMP( "Idle Task Creation failed %d" , ret );
    return ret;  
  }
  DUMP( "Idle Task Created Task %p" , idle_task );


  //- Create the first user mode task                         -//
  ret = vmm_init_task_vm( idle_task, &init_task );
  if( ret != KERN_SUCCESS )  {
    DUMP( "init task Creation failed %d" , ret );
    return ret;  
  }
  DUMP( "init task Created Task %p" , init_task );


  //- Set up hardcoded binary user image for the init task    -//
  ret = task_setup_init_code( init_task , initial_binary );
  if( ret != KERN_SUCCESS )  {
    DUMP( "init task code setup failed %d" , ret );
    return ret;  
  }
  DUMP( "init task code setup done!" );

  //- add init task to scheduler queue -//
  scheduler_add( &init_task->initial_thread );


  //-- We never add the idle thread to the run queue     --//
  //-- Its different because it has to be run when       --//
  //-- there is NO thread. adding it to the run queue    --//
  //-- adds special case to the our simple pop first     --//
  //-- scheduler                                         --//
  
  //-- will result in first call to schedule->init task  --//
  task_run_idle_loop( idle_task );

  panic("Kernel has returned from idle loop");
  FN_LEAVE();
  return ret;
}
