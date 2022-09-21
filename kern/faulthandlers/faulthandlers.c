/** @file     faulthandlers.c 
 *  @brief    This file contains the faulthandler initializations
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
#include <faulthandlers.h>
#include <task.h>

#define MAX_FAULT_HANDLERS     20
#define PAGE_FAULT_REASON_IDX  -6

#define SRC_PAGE_MAP_IDX       0
#define DST_PAGE_MAP_IDX       1
char    *copy_area;


#define FAULT_ACTION_KILL         0
#define FAULT_ACTION_COW          1
#define FAULT_ACTION_BACK_PAGES   3
#define FAULT_ACTION_GROW_STACK   4
#define FAULT_ACTION_PANIC        5


/** @macro    TOTAL_FAULT_HANDLERS
 *  @brief    total fault handlers supported by the kernel 
 */

#define TOTAL_FAULT_HANDLERS (FAULT_XF - FAULT_DE + 1) // -- 20 -- //

/** @typedef   GENERIC_FUNCTION_CALL_ADDRESS
 *  @brief     generic function call address type for the faulthandlers
 */

typedef void (*GENERIC_FN_CALL_ADDRESS)();

// -- fault handler function prototypes -- //
void  static fault_generic();
void  static fault_generic_fatal();
void  static divby0_fault_handler();
void  static alignment_fault_handler();
void  static opcode_fault_handler();
void  static device_fault_handler();
void  static double_fault_handler();
void  static page_fault_handler();



typedef struct _HANDLE_FAULT { 
  int   fault_nr;
  GENERIC_FN_CALL_ADDRESS fn_address; 
}HANDLE_FAULT;

/** @global   handle_fault_table
 *  @brief    dispatch table for faulthandlers
 */
HANDLE_FAULT faulthandler_table[TOTAL_FAULT_HANDLERS] = 
  {
    { FAULT_DE               , divby0_fault_handler },
    { FAULT_DB               , fault_generic }, // -- debugmode not enabled -- //
    { FAULT_NMI              , fault_generic }, // -- NMI not handled -- //
    { FAULT_BP               , fault_generic }, // -- Breakpts not enabled -- //
    { FAULT_OF               , fault_generic }, // -- Overflow not handled -- //
    { FAULT_BR               , fault_generic }, // -- Bounds check not made -- //
    { FAULT_UD               , opcode_fault_handler }, 
    { FAULT_NM               , device_fault_handler }, 
    { FAULT_DF               , double_fault_handler },  
    { FAULT_CSO              , fault_generic_fatal }, // -- kill thread -- //
    { FAULT_TS               , fault_generic_fatal }, // -- kill thread -- //
    { FAULT_NP               , fault_generic_fatal }, // -- kill thread -- //
    { FAULT_SS               , page_fault_handler },
    { FAULT_GP               , page_fault_handler },
    { FAULT_PF               , page_fault_handler },
    { FAULT_RESERVED         , fault_generic },
    { FAULT_MF               , fault_generic },  // -- flpt not handled in kernel -- //
    { FAULT_AC               , alignment_fault_handler }, 
    { FAULT_MC               , fault_generic_fatal },  // -- bus errors -- //
    { FAULT_XF               , fault_generic }  // -- kernel doesn't support SIMD -- //
  };



    
/** @function  invalidate_tlb
 *  @brief     This function invalidates the tlb using the asm INVLPG instruction
 *  @param     addr - faulting address requiring action
 *  @return    void
 */

static inline void invalidate_tlb(unsigned long addr)
{
        asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
    
/** @function  relocate_iret_frame
 *  @brief     This function relocates the iret frame setup on top of kernel stack
 *             so as to be appropriately handled by suitable faulthandler
 *  @param     none
 *  @return    void
 */

static inline void relocate_iret_frame() {
  kthread *thisThread = CURRENT_THREAD;
  int i;
  for(i=PAGE_FAULT_REASON_IDX; i <= -1;i++)
    thisThread->context.kstack[i] = thisThread->context.kstack[i+1];
}


/*
  Following cases                              Action     
  1. Write on COW Pages                        Allocate new page & copy 
  2. Stack Growth.                             Allocate new page
  3. Allocated Pages Accessed                  Back with new page
  3. Trying to access kernel memory            Kill
  4. Trying to access un-mapped random memory  Kill
  5. Write on RO section.                      Kill
  6. Fault in Kernel mode                      Panic.
*/

/** @function  analyse_fault
 *  @brief     This function identifies the reason why the fault occurred
 *             and appropriately returns the fault reason to the caller
 *  @param     reason         - page table entry for the faulting address
 *  @param     linear_address - linear address which caused the fault
 *  @return    encoded integer value representing the type of fault
 */

int static analyse_fault(PTE reason,
			 uint32_t linear_address) 
{
  vm_range *vm_range_ptr;
  vm_range_ptr = vmm_get_range(&CURRENT_THREAD->pTask->vm,
			       (char *)linear_address);

  //-- Writing to read only memory --//
  if(reason.PRESENT && 
     reason.RW &&
     vmm_is_address_ro(&CURRENT_THREAD->pTask->vm,(void *)linear_address))
    return FAULT_ACTION_KILL;

  //-- Kernel address fault --//
  if(linear_address < USER_MEM_START)
    return FAULT_ACTION_KILL;

  //- alloced ranges being accessed -//
  if(0 == reason.PRESENT && 
     NULL != vm_range_ptr)
     return FAULT_ACTION_BACK_PAGES;
  
  //- Copy on write page being accessed -//
  if(1 == reason.PRESENT && vm_range_ptr) 
    return FAULT_ACTION_COW;


  //- We will not get the VMRange in case stack is growing -//
  if(NULL == vm_range_ptr && (unsigned long)linear_address ==  
     CURRENT_THREAD->pTask->vm.vm_stack_start - 1) {
    return FAULT_ACTION_GROW_STACK;
  }

  return FAULT_ACTION_KILL;
}


/** @function  _BASE_FAULT_HANDLER
 *  @brief     This function sets up the handler actions
 *             and appropriately executes a suitable handler for the analyzed fault
 *  @param     none
 *  @return    void
 */

void static page_fault_handler(void) {
  KERN_RET_CODE  ret;
  vm_range *vmrange_ptr;
  kthread *thisThread = CURRENT_THREAD;
  PTE      reason,attr;
  PTE     *faulting_pte;
  PTE     *faulting_pde;
  uint32_t linear_address;
  PTE     *src_dst_pte;
  PFN      newPageFrame;
  ktask   *task;
  PTE     *new_pte=NULL;
  LINEAR_ADDRESS_BREAKER linear_address_b;
  char errmsg[200];



  //- collect all information --//
  reason = *(PTE *)(thisThread->context.kstack+PAGE_FAULT_REASON_IDX);
  relocate_iret_frame();
  linear_address = (uint32_t) get_cr2();
  faulting_pte = vmm_get_pte(&thisThread->pTask->vm,linear_address);
  faulting_pde = vmm_get_pde(&thisThread->pTask->vm,linear_address);
  src_dst_pte  = vmm_get_pte(&thisThread->pTask->vm,(uint32_t)copy_area);

  //DUMP("IN PAGE FAULTHANDLERS for thread %p stack %p %p",
  //     thisThread,thisThread->context.kstack,(char *)linear_address);

  //- setup copy regions      --//
  src_dst_pte[SRC_PAGE_MAP_IDX].PRESENT = 1;
  src_dst_pte[SRC_PAGE_MAP_IDX].RW      = 1;
  src_dst_pte[SRC_PAGE_MAP_IDX].US      = 0;
  src_dst_pte[SRC_PAGE_MAP_IDX].WT      = 1;
  src_dst_pte[SRC_PAGE_MAP_IDX].ADDRESS = 0;

  src_dst_pte[DST_PAGE_MAP_IDX].PRESENT = 1;
  src_dst_pte[DST_PAGE_MAP_IDX].RW      = 1;
  src_dst_pte[DST_PAGE_MAP_IDX].US      = 0;
  src_dst_pte[DST_PAGE_MAP_IDX].WT      = 1;
  src_dst_pte[DST_PAGE_MAP_IDX].ADDRESS = 0;


  switch(analyse_fault(reason,linear_address)) {
  case FAULT_ACTION_GROW_STACK: 
    //- simple action for now - just extends the stack range by 1 page downward -//

    linear_address_b.address = (unsigned long)linear_address;

    //-- Install the PTE entryin PDE if its not already --//
    if(!CURRENT_THREAD->pTask->vm.pde_base[linear_address_b.u.PDE_IDX].PRESENT) {
        new_pte = smemalign( PAGE_SIZE , PAGE_SIZE );
	if(!new_pte) {
	  ret = KERN_NO_MEM;
	  DUMP("Cannot grow stack low on memory");
	  goto action_kill;
	}
	memset( new_pte , 0 , PAGE_SIZE );

	//-- set up the entry in PDE --//
	//-- this function sets up the tables and does not cares about
	//-- protection bits caller must set it up using VMM_get_pte
	CURRENT_THREAD->pTask->vm.pde_base[linear_address_b.u.PDE_IDX].PRESENT = 1;
	CURRENT_THREAD->pTask->vm.pde_base[linear_address_b.u.PDE_IDX].ADDRESS =
	  (unsigned long)new_pte >> PAGING_PAGE_OFFSET_BITS;
    }


    //- adjust the kstack vmm_range -//
    vmrange_ptr = vmm_get_range(&thisThread->pTask->vm,
				(char *)thisThread->pTask->vm.vm_stack_start);
    assert(NULL != vmrange_ptr);
    if(NULL == vmrange_ptr)
      goto action_kill;

    vmrange_ptr->start -= PAGE_SIZE;
    thisThread->pTask->vm.vm_stack_start -= PAGE_SIZE;

    //- set up the stack range attributes again --//
    memset(&attr,0,sizeof(attr));
    attr.PRESENT      = 1;
    attr.RW           = 1;
    attr.US           = 1;
    attr.GLOBAL       = 0;
    vmm_set_range_attr( &thisThread->pTask->vm , vmrange_ptr , attr );


    faulting_pte = vmm_get_pte(&thisThread->pTask->vm,linear_address);
    faulting_pde = vmm_get_pde(&thisThread->pTask->vm,linear_address);
    invalidate_tlb(linear_address);

    //- fall through to back the page -//
  case FAULT_ACTION_BACK_PAGES:
    ret = vmm_get_free_user_pages(&newPageFrame); 
    if( KERN_SUCCESS != ret ){
      DUMP("No free pages to perform backing");
      goto action_kill;
    }
    faulting_pte->PRESENT = 1;
    faulting_pte->ADDRESS = newPageFrame;
    faulting_pte->RW      = 1;
    invalidate_tlb(linear_address);
    //- Zero out the page -//
    memset((char *)((unsigned long)linear_address & ~PAGE_MASK),
	   0,
	   PAGE_SIZE);
    return;
    break; 

  case FAULT_ACTION_COW:
    //- map in src actual page              -//
    src_dst_pte[SRC_PAGE_MAP_IDX].ADDRESS = faulting_pte->ADDRESS;
    
    //- allocate and map in new page        -//
    ret = vmm_get_free_user_pages(&newPageFrame); 
    if( KERN_SUCCESS != ret ){
      DUMP("No free pages to perform copy on write");
      goto action_kill;
    }
    src_dst_pte[DST_PAGE_MAP_IDX].ADDRESS = newPageFrame;

    //- copy                                   -//
    invalidate_tlb((unsigned long)copy_area);
    invalidate_tlb((unsigned long)copy_area+PAGE_SIZE);
    
    memcpy(copy_area+PAGE_SIZE,copy_area,PAGE_SIZE);

    faulting_pte->ADDRESS = newPageFrame;
    if(vmm_is_address_ro(&CURRENT_THREAD->pTask->vm,(void *)linear_address)) {

      faulting_pte->RW = 0;
      faulting_pde->RW = 0;
    }else {
      faulting_pte->RW = 1;
      faulting_pde->RW = 1;
    }
    break;

  case FAULT_ACTION_PANIC:
    panic("KERNEL_PANIC: Bad Thing happened as Address %p",(char *)linear_address);
    break;

  case FAULT_ACTION_KILL:
    action_kill:
    task = (CURRENT_THREAD)->pTask;
    DUMP("killing thread %p faulted at address %p",CURRENT_THREAD , (char *)linear_address);
    (CURRENT_THREAD)->run_flag = -1;
    
    //    if( &task->initial_thread == CURRENT_THREAD )
    //  vmm_free_task_vm( task);
    
    Q_REMOVE( &task->ktask_threads_head , (CURRENT_THREAD) , kthread_next );
    if(task->ktask_threads_head.nr_elements == 0) {
      task->state = TASK_STATUS_ZOMIE;
      //- we cannot be scheduled anymore now -//

      sprintf(errmsg, "FATAL: killing thread %p on invalid access of memory address %p\n", CURRENT_THREAD , (char *)linear_address);
      errmsg[strlen(errmsg)] = '\0';
      putbytes(errmsg,strlen(errmsg));
      
      sem_signal(&task->parentTask->vultures);
    }

    schedule(CURRENT_NOT_RUNNABLE); // -- yield to next runnable thread -- //
    break;
  }

  invalidate_tlb(linear_address);
  return;

}




/** @function  fault_generic_fatal
 *  @brief     This function prints debug messages on kernel log
 *             and goes on to terminate the thread
 *  @return    void
 */

void static fault_generic_fatal() {

  ktask *task;
  kthread *thread;

  FN_ENTRY();
  task = (CURRENT_THREAD)->pTask;
  (CURRENT_THREAD)->run_flag = -1;

  DUMP("FATAL FAULT : Killing thread %p",  CURRENT_THREAD );

  // -- remove the current faulted thread from the task queue -- //
  Q_REMOVE( &task->ktask_threads_head , (CURRENT_THREAD) , kthread_next );

  // -- if current thread is initial thread -- //
  if( &task->initial_thread == CURRENT_THREAD ) {
    // -- prepare to kill the entire task -- //
    // -- free the allocated memory for all forked threads (force kill) -- //
    Q_FOREACH( thread , &task->ktask_threads_head , kthread_next ) {
      // -- dequeue the thread from the task thread queue -- // 
      Q_REMOVE( &task->ktask_threads_head , thread , kthread_next );

      // -- just free the kernel stack of the forked thread -- //
      sfree( thread , PAGE_SIZE * KTHREAD_KSTACK_PAGES );
    }

  }

  if(task->ktask_threads_head.nr_elements == 0) {
    task->state = TASK_STATUS_ZOMIE;

    sem_signal(&task->parentTask->vultures);
  } 
  else // -- just free the kernel stack of the forked thread -- //
    sfree( (CURRENT_THREAD) , PAGE_SIZE * KTHREAD_KSTACK_PAGES );

  schedule(CURRENT_NOT_RUNNABLE); // -- deschedule self and yield to next runnable thread -- //

  FN_LEAVE();
}



/** @function  divby0_fault_handler
 *  @brief     This function handles the fault - divide by zero
 *  @return    void
 */

void static divby0_fault_handler() {
  char errmsg[100];
  FN_ENTRY();

  sprintf(errmsg, "DIVIDE BY ZERO!!!\nKilling thread %p\n", CURRENT_THREAD );
  errmsg[strlen(errmsg)] = '\0';
  putbytes(errmsg,strlen(errmsg));

  fault_generic_fatal( );
  
  FN_LEAVE();

}

/** @function  alignment_fault_handler
 *  @brief     This function handles the fault - memory alignment failure
 *  @return    void
 */

void static alignment_fault_handler() {
  char errmsg[100];
  FN_ENTRY();

  sprintf(errmsg, "ALIGNMENT CHECK FAILED!!!\nKilling thread %p\n", CURRENT_THREAD );
  errmsg[strlen(errmsg)] = '\0';
  putbytes(errmsg,strlen(errmsg));

  fault_generic_fatal( );
  
  FN_LEAVE();

}

/** @function  opcode_fault_handler
 *  @brief     This function handles the fault - invalid opcode
 *  @return    void
 */

void static opcode_fault_handler() {
  char errmsg[100];
  FN_ENTRY();

  sprintf(errmsg, "INVALID OPCODE!!!\nKilling thread %p\n", CURRENT_THREAD );
  errmsg[strlen(errmsg)] = '\0';
  putbytes(errmsg,strlen(errmsg));

  fault_generic_fatal( );
  
  FN_LEAVE();

}

/** @function  device_fault_handler
 *  @brief     This function handles the fault - device not present
 *             Thread is trying to access a device that doesn't exist 
 *  @param     fault_type - type of fault (offset of IDT entry for that fault)
 *  @return    void
 */

void static device_fault_handler() {
  char errmsg[100];
  FN_ENTRY();

  sprintf(errmsg, "DEVICE NOT PRESENT!!!\nKilling thread %p\n", CURRENT_THREAD );
  errmsg[strlen(errmsg)] = '\0';
  putbytes(errmsg,strlen(errmsg));
  
  fault_generic_fatal( );
  
  FN_LEAVE();

}

/** @function  double_fault_handler
 *  @brief     This function handles the fault - double fault
 *  @param     fault_type - type of fault (offset of IDT entry for that fault)
 *  @return    void
 */

void static double_fault_handler() {
  char errmsg[100];
  FN_ENTRY();

  sprintf(errmsg, "DOUBLE_FAULT!!!\nKilling thread %p\n", CURRENT_THREAD );
  errmsg[strlen(errmsg)] = '\0';
  putbytes(errmsg,strlen(errmsg));

  fault_generic_fatal();
  FN_LEAVE();
}

/** @function  fault_generic
 *  @brief     This function prints debug messages on kernel log
 *  @param     fault_type - type of fault (offset of IDT entry for that fault)
 *  @return    void
 */

void static fault_generic( int fault_type ) {

  FN_ENTRY();
  DUMP("ENCOUNTERED FAULT (%d): Thread %p", fault_type , CURRENT_THREAD );
  FN_LEAVE();

}


/** @function  faulthandler_init
 *  @brief     This function installs the faulthandlers ISR in IDT
 *  @param     none
 *  @return    KERN_SUCCESS if success
 *             KERN error code if failure
 */

KERN_RET_CODE faulthandler_init() {
  int i;
  KERN_RET_CODE ret;
  FN_ENTRY();

  //-- make some room for copy area --//
  copy_area = smemalign(PAGE_SIZE,PAGE_SIZE * 2);
  if(!copy_area)
    return KERN_NO_MEM;

  //-- Install the handlers --//
  for(i=0;i<20;i++) {
    if(i == FAULT_DF) 
      continue; 

    ret = i386_install_isr((void *) (faulthandler_table[i].fn_address),
			   faulthandler_table[i].fault_nr,
			   i386_GATE_TYPE_TRAP,
			   i386_PL0);
    if(ret != KERN_SUCCESS)  
      return ret;
  }
  
  FN_ENTRY();
  return KERN_SUCCESS;
}
