/** @file     vmm.c
 *  @brief    This file contains the VMM helper routines
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
#include <cr.h>
#include <kern_common.h>
#include <syscall_int.h>
#include <syscall_entry.h>
#include "i386lib/i386systemregs.h"


//-- the vm manager context for the kernel --//
kern_vmm kernel_vmm;

/** @function  vmm_get_free_user_pages
 *  @brief     This function is used to get user page frames
 *  @param     pfn - pointer to frame number holder
 *  @return    KERN_SUCCESS on successful get; else KERN_NO_MEM
 */

KERN_RET_CODE vmm_get_free_user_pages(PFN *pfn) {
  int i;

  for(i=KERNEL_PAGES_NR+1;i<kernel_vmm.nr_physical_pages;i++) {
    assert( kernel_vmm.m_pages[i].refcount >= 0 );
    if(!kernel_vmm.m_pages[i].refcount) {
      kernel_vmm.m_pages[i].refcount = 1;
      *pfn = (PFN) i;
      return KERN_SUCCESS;
    }
  }

  return KERN_NO_MEM;
}

/** @function  vmm_getref_user_page
 *  @brief     This function is used to increment refcount for a frame - share (COW)
 *  @param     pfn - frame number holder
 *  @return    void
 */

void vmm_getref_user_page(PFN pfn) {
  assert(pfn < kernel_vmm.nr_physical_pages);
  assert(pfn >= KERNEL_PAGES_NR + 1);

  assert(kernel_vmm.m_pages[pfn].refcount >= 1);
  // -- there is one reference from getfreepages -- //

  kernel_vmm.m_pages[pfn].refcount++;
}

/** @function  vmm_putref_user_page
 *  @brief     This function is used to decrement refcount for a frame - unshare (COW)
 *  @param     pfn - frame number holder
 *  @return    void
 */

void vmm_putref_user_page(PFN pfn) {
  assert(pfn < kernel_vmm.nr_physical_pages);
  assert(pfn >= KERNEL_PAGES_NR + 1);
  kernel_vmm.m_pages[pfn].refcount--;
  assert(kernel_vmm.m_pages[pfn].refcount >= 0);
}

/** @function  vmm_init_task_vm
 *  @brief     This function is used to initialize a task's VM
 *  @param     parentTask - pointer to the parentTask that forks new task
 *  @param     ppTask - pointer to the placeholder of current task
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_init_task_vm(ktask  *parentTask,
			       ktask **ppKTask) {
  char *taskmem;
  int  totalTaskAllocation;
  int  i;
  int  initial_pte_pages;

  PTE    *pde_base;
  PTE    *pte_base;
  ktask  *newTask;

  FN_ENTRY();

  //- Calculate the PTE pages required for covering the kernel range--//
  initial_pte_pages = KERNEL_PAGES_NR / PTE_PER_PAGE;
  if( KERNEL_PAGES_NR % PTE_PER_PAGE )
    initial_pte_pages =+ 1;


  //-- Get the total pages need to create a task --//
  totalTaskAllocation = KTHREAD_KSTACK_PAGES  + INITIAL_PDE_PAGES
    + initial_pte_pages;
  totalTaskAllocation *= PAGE_SIZE;

  taskmem = smemalign(PAGE_SIZE * KTHREAD_KSTACK_PAGES ,totalTaskAllocation);
  //DUMP("Alloc task mem %p",taskmem);
  if( NULL == taskmem )
    return KERN_NO_MEM;
  memset(taskmem,0,totalTaskAllocation);

  //-- Fix up pointers upfront --//
  pde_base = (PTE *) (taskmem  + (PAGE_SIZE * KTHREAD_KSTACK_PAGES));
  pte_base = (PTE *) ((char *) pde_base +  (PAGE_SIZE * INITIAL_PDE_PAGES));


  //--  the PDE page --//
  for(i=0 ; i < initial_pte_pages ; i++) {
    pde_base[i].PRESENT         = 1;
    pde_base[i].RW              = 1;
    pde_base[i].US              = 0;
    pde_base[i].WT              = 0;
    pde_base[i].CACHE_DISABLED  = 0;
    pde_base[i].ACCESSED        = 0;
    pde_base[i].DIRTY           = 0;
    pde_base[i]._PAGE_SIZE      = 0;
    pde_base[i].GLOBAL          = 1;
    pde_base[i].AVAIL           = 0;
    //- Initial PTE's are contigious -//
    pde_base[i].ADDRESS         = ((unsigned long)pte_base & ~(PAGE_MASK)) >> PAGING_PAGE_OFFSET_BITS;
    pde_base[i].ADDRESS        += i;
  }

  //-- the direct mapped PTE pages --//
  for(i=0;i<KERNEL_PAGES_NR;i++) {
    pte_base[i].PRESENT         = 1;
    pte_base[i].RW              = 1;
    pte_base[i].US              = 0;
    pte_base[i].WT              = 0;
    pte_base[i].CACHE_DISABLED  = 0;
    pte_base[i].ACCESSED        = 0;
    pte_base[i].DIRTY           = 0;
    pte_base[i]._PAGE_SIZE      = 0;
    pte_base[i].GLOBAL          = 1;
    pte_base[i].AVAIL           = 0;

    //- Direct Map -//
    pte_base[i].ADDRESS         = i;
  }

  //-- Store the task struct on the stack itself -//
  newTask  = (ktask *) taskmem;


  //-- Initialize the task struct now --//
  //-- Task VM Init                   --//
  newTask->vm.taskmem = taskmem;
  newTask->vm.totalTaskAllocation = totalTaskAllocation;
  newTask->vm.pde_base = pde_base;

  //- To hook up the kernel range --//

  //-- Task Initial thread Init      --//
  newTask->initial_thread.context.kstack =
    (STACK_ELT *)(taskmem + (PAGE_SIZE * KTHREAD_KSTACK_PAGES));
  newTask->initial_thread.context.kstack--; // GUARD
  newTask->initial_thread.context.kstack--; // GUARD
  newTask->initial_thread.context.kstack--; // GUARD
  newTask->initial_thread.context.kstack--; // GUARD


  newTask->initial_thread.context.r_esp =
    newTask->initial_thread.context.kstack;


  //-- Set up the KERNEL_VMM range -//
  newTask->vm.vm_range_kernel.start = 0x0;
  newTask->vm.vm_range_kernel.len   = USER_MEM_START;
  Q_INIT_HEAD(&newTask->vm.vm_ranges_head);
  Q_INIT_ELEM(&newTask->vm.vm_range_kernel , vm_range_next);
  Q_INSERT_FRONT( &newTask->vm.vm_ranges_head  ,
		  &newTask->vm.vm_range_kernel ,
		  vm_range_next);


  //-- Initialize list of children & wait semaphore--//
  SEMAPHORE_INIT( &newTask->vultures , 0);

  //-- hook up the initial thread into the thread list --//
  Q_INIT_HEAD( &newTask->ktask_threads_head );
  Q_INIT_ELEM( &newTask->initial_thread , kthread_next);
  Q_INSERT_FRONT( &newTask->ktask_threads_head,
		  &newTask->initial_thread,
		  kthread_next);
  newTask->initial_thread.pTask = newTask;

  //-- Initialize the waiting list --//
  Q_INIT_ELEM( &newTask->initial_thread , kthread_wait );


  //- set up parent child -//
  SEMAPHORE_INIT( &newTask->fork_lock , 1);
  Q_INIT_HEAD( &newTask->ktask_task_head );
  Q_INIT_ELEM( newTask , ktask_next);
  newTask->parentTask = parentTask;

  //-- parent is NULL for the init task --//
  if(NULL != parentTask) {
    Q_INSERT_FRONT( &parentTask->ktask_task_head,
		    newTask,
		    ktask_next);

    //- copy over book keeping -//
  newTask->vm.vm_text_start = parentTask->vm.vm_text_start;
  newTask->vm.vm_text_len  = parentTask->vm.vm_text_len;

  newTask->vm.vm_data_start = parentTask->vm.vm_data_start;
  newTask->vm.vm_data_len   = parentTask->vm.vm_data_len;

  newTask->vm.vm_rdata_start = parentTask->vm.vm_rdata_start;
  newTask->vm.vm_rdata_len   = parentTask->vm.vm_rdata_len;

  newTask->vm.vm_stack_start = parentTask->vm.vm_stack_start;
  newTask->vm.vm_stack_len   = parentTask->vm.vm_stack_len;
  }





  *ppKTask = newTask;
  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  vmm_free_all_vma
 *  @brief     This function is used to destroy the vm ranges of a given task
 *  @param     vm_dst - pointer to the VMmanager of a task which will be destroyed
 *  @return    void
 */

void vmm_free_all_vma( struct task_vm *vm_dst ) {
  vm_range *vmrange_ptr,*vmrange_ptr_save;
  Q_FOREACH_DEL_SAFE(vmrange_ptr,
		     &vm_dst->vm_ranges_head,
		     vm_range_next,
		     vmrange_ptr_save)  {
    if(vmrange_ptr == &vm_dst->vm_range_kernel)
      continue;

    Q_REMOVE(&vm_dst->vm_ranges_head,
	     vmrange_ptr,
	     vm_range_next);
    free(vmrange_ptr);
  }
}


/** @function  vmm_free_task_vm
 *  @brief     This function is used to destroy the entire task in memory
 *  @param     pTask - pointer to the Task that will be destroyed
 *  @return    void
 */
void vmm_free_task_vm(ktask *pTask) {
  KERN_RET_CODE ret;
  FN_ENTRY();

  //- Release all references to user mode pages -//
  ret = vmm_unback_all_user_ranges(&pTask->vm);
  if(ret != KERN_SUCCESS) {
    DUMP("Memory leaked in vmm_unback_all_user_ranges");
    ret = KERN_SUCCESS;
  }

  //- Release all use mode PTE directories  -//
  ret = vmm_free_user_ptes(&pTask->vm);
  if(ret != KERN_SUCCESS) {
    DUMP("Memory leaked in vmm_unback_all_user_ranges 2");
    ret = KERN_SUCCESS;
  }

  //- Release all user mode vm area structs -//
  vmm_free_all_vma(&pTask->vm);

  //- finally release kernel mode PDE directory and PTE directories -//
  sfree(pTask->vm.taskmem,pTask->vm.totalTaskAllocation);
  FN_LEAVE();
}


//-- --//
#define MINIMUM_PAGES_TO_OPERATE 12

/** @function  vmm_init
 *  @brief     This function is used to initialize the VMmanager globally
 *             It maintains the physical frames available in RAM
 *             and divides them between the kernel and the user
 *  @param     none
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_init(void) {
  FN_ENTRY();

  memset(&kernel_vmm,0,sizeof(kernel_vmm));


  //- Get the total number of pages available --//
  kernel_vmm.nr_physical_pages = machine_phys_frames();
  DUMP("vmm_init Machine reported 0x%x pages of size 0x%x",
       kernel_vmm.nr_physical_pages , PAGE_SIZE );

  //-- We need have a minumum memory pages knob --//
  if( kernel_vmm.nr_physical_pages <
      KERNEL_PAGES_NR + MINIMUM_PAGES_TO_OPERATE ) {
    FN_LEAVE();
    return KERN_NO_MEM;
  }

  kernel_vmm.nr_free_pages -= KERNEL_PAGES_NR;

  //-- Start the page manager --//
  kernel_vmm.next_free_page = KERNEL_PAGES_NR + 1;

  //-- allocate the mpage structs --//
  kernel_vmm.m_pages = malloc(sizeof(m_page) * kernel_vmm.nr_physical_pages);
  if( !kernel_vmm.m_pages ){
    FN_LEAVE();
    return KERN_NO_MEM;
  }
  memset(kernel_vmm.m_pages,0,sizeof(m_page) * kernel_vmm.nr_physical_pages);
  FN_LEAVE();
  return KERN_SUCCESS;
}



//-- Address space manipulations                     --//
//-- Ranges need to start and end at page boundaries --//
//-- Input task_vm with atleast PDBR                 --//

/** @function  vmm_install_range
 *  @brief     This function is used to install
 *             the supplied range into the task's VM
 *  @param     address_space - pointer to the task's VM
 *  @param     range         - pointer to the range that must be installed
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_install_range(struct task_vm *address_space,
				vm_range *range)
{
  KERN_RET_CODE ret;
  unsigned long range_end;
  vm_range      *new_range=NULL;
  PTE           *new_pte=NULL;
  int pages_nr;
  int i;
  FN_ENTRY();

  assert(NULL != address_space->pde_base);

  range_end = range->start + range->len;


  //-- Scale start down to page boundary --//
  range->start = range->start & (unsigned long)(~PAGE_MASK);

  //-- Scale end up to page boundary     --//
  range_end   += (PAGE_SIZE - 1);
  range_end   =  range_end & (unsigned long)(~PAGE_MASK);

  //- get the scaled lenght --//
  range->len   = range_end - range->start;


  if( range->start < USER_MEM_START ) {
    return KERN_ERROR_VM_CANNOT_MAP;
  }


  //-- allocate the range structure --//
  new_range = malloc(sizeof(*range));
  if(!new_range)
    return KERN_NO_MEM;

  //-- Insert the range on to the list of available ranges --//
  new_range->start = range->start;
  new_range->len   = range->len;
  Q_INIT_ELEM( new_range , vm_range_next);
  Q_INSERT_FRONT( &address_space->vm_ranges_head ,
		  new_range ,
		  vm_range_next );


  //-- PDE has to be installed always because its create using
  //-- vmm_init_task_vm()
  pages_nr = new_range->len / PAGE_SIZE;
  assert(pages_nr);

  //-- Install the missing PTE ranges --//
  for( i = 0 ; i < pages_nr ; i++ )  {
    LINEAR_ADDRESS_BREAKER linear_address;
    linear_address.address = range->start + (i * PAGE_SIZE);

    //-- Install the PTE entryin PDE if its not already --//
    if(!address_space->pde_base[linear_address.u.PDE_IDX].PRESENT) {
        new_pte = smemalign( PAGE_SIZE , PAGE_SIZE );
	if(!new_pte) {
	  ret = KERN_NO_MEM;
	  goto error;
	}
	memset( new_pte , 0 , PAGE_SIZE );

	//-- set up the entry in PDE --//
	//-- this function sets up the tables and does not cares about
	//-- protection bits caller must set it up using VMM_get_pte
	address_space->pde_base[linear_address.u.PDE_IDX].PRESENT = 1;
	address_space->pde_base[linear_address.u.PDE_IDX].ADDRESS =
	  (unsigned long)new_pte >> PAGING_PAGE_OFFSET_BITS;
    }
  }

  return KERN_SUCCESS;

error:
  if( new_pte )
    sfree(new_pte,PAGE_SIZE);

  assert( new_range );
  Q_REMOVE( &address_space->vm_ranges_head ,
	    new_range ,
	    vm_range_next );
  free(new_range);

   FN_LEAVE();
  return ret;
}

/** @function  invalidate_tlb
 *  @brief     This function invalidates the tlb using the asm INVLPG instruction
 *  @param     addr - faulting address requiring action
 *  @return    void
 */

static inline void invalidate_tlb(unsigned long addr)
{
        asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}


/** @function  vmm_uninstall_range
 *  @brief     This function is used to uninstall
 *             the supplied range from the task's VM
 *  @param     address_space - pointer to the task's VM
 *  @param     range         - pointer to the range that must be deinstalled
 *                             (range->start must be populated before call)
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_uninstall_range(struct task_vm *address_space,
				  vm_range *range)
{
  PTE *pte;
  unsigned long linear_address;
  vm_range *vmrange_ptr;
  FN_ENTRY();

  Q_FOREACH( vmrange_ptr , &address_space->vm_ranges_head , vm_range_next )  {

    if(vmrange_ptr == &address_space->vm_range_kernel)
      continue;

    if(vmrange_ptr->start == range->start &&
       vmrange_ptr->len == range->len)
      break;
  }

  //-- we don't have info about the range to be unmapped --//
  if(!vmrange_ptr || vmrange_ptr->start != range->start) {
    return KERN_ERROR_ADDRESS_NOT_PRESENT;
  }


  //-- unback all the pages pointed to by this VM range -//
  for(linear_address = vmrange_ptr->start;
      linear_address < (vmrange_ptr->start + vmrange_ptr->len);
      linear_address += PAGE_SIZE) {

    pte = vmm_get_pte(address_space,linear_address);
    assert(pte);

    if(pte->PRESENT) {
      pte->PRESENT = 0;
      vmm_putref_user_page(pte->ADDRESS);
      pte->ADDRESS = 0;
      invalidate_tlb(linear_address);
    }

  }


  //- Free book keeping information -//
  Q_REMOVE(&address_space->vm_ranges_head,
	   vmrange_ptr,
	   vm_range_next);
  free(vmrange_ptr);

  FN_LEAVE();
  return KERN_SUCCESS;
}


//-- WE don't have an corresponding free kernel mod pte --//
//-- because we never do that                           --//

/** @function  vmm_copy_user_ptes
 *  @brief     This function is used to copy over the userland pte entries
 *             from the source VM to the destination VM
 *  @param     address_space_dst - pointer to destination task's VM
 *  @param     address_space_src - pointer to source task's VM
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_copy_user_ptes( struct task_vm *address_space_dst ,
				  struct task_vm *address_space_src )
{
  int i;
  uint32_t address =  USER_MEM_START;
  FN_ENTRY();
  LINEAR_ADDRESS_BREAKER la;
  la.address = address;

  for(i=la.u.PDE_IDX ; i  < 1024 ; i++)
    address_space_dst->pde_base[i] = address_space_src->pde_base[i];


  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  vmm_free_user_ptes
 *  @brief     This function is used to free over the userland pte entries
 *             from the task's VM
 *  @param     address_space - pointer to task's VM
 *  @return    KERN_SUCCESS on success; KERN err code on failure
 */

KERN_RET_CODE vmm_free_user_ptes( struct task_vm *address_space )
{
  FN_ENTRY();
  int i;
  uint32_t address =  USER_MEM_START;
  FN_ENTRY();
  LINEAR_ADDRESS_BREAKER la;
  la.address = address;

  for(i=la.u.PDE_IDX ; i  < 1024 ; i++) {
    if(address_space->pde_base[i].PRESENT) {
      unsigned long pte_addr;
      pte_addr = (unsigned long) address_space->pde_base[i].ADDRESS <<  PAGING_PAGE_OFFSET_BITS;

      sfree((char *) pte_addr,
	    PAGE_SIZE);
      address_space->pde_base[i].ADDRESS = 0;
      address_space->pde_base[i].PRESENT = 0;
    }
  }

  FN_LEAVE();
  return KERN_SUCCESS;
}

/** @function  vmm_get_pte
 *  @brief     This function is used to get the PageTable entry encountered
 *             when accessing the giver user address
 *  @param     address_space - pointer to task's VM
 *  @param     address       - the userland address in question
 *  @return    pointer to the PTE
 */

PTE *vmm_get_pte( struct task_vm *address_space , uint32_t address )
{
  LINEAR_ADDRESS_BREAKER linear_address;

    FN_ENTRY();
    linear_address.address = address;

    assert(address_space->pde_base);
    if(address_space->pde_base[linear_address.u.PDE_IDX].PRESENT) {
      PTE *pte_page = (PTE *)
	(unsigned long) address_space->pde_base[linear_address.u.PDE_IDX].ADDRESS;

      pte_page = (PTE *)
	  ((unsigned long)pte_page << PAGING_PAGE_OFFSET_BITS);

	//-- return the PTE entry --//
	FN_LEAVE();
	return &pte_page[linear_address.u.PTE_IDX];
    }else {
      FN_LEAVE();
      return NULL;
    }
}

/** @function  vmm_get_pde
 *  @brief     This function is used to get the PageDir entry encountered
 *             when accessing the giver user address
 *  @param     address_space - pointer to task's VM
 *  @param     address       - the userland address in question
 *  @return    pointer to the PDE
 */

PDE *vmm_get_pde( struct task_vm *address_space , uint32_t address )
{
  LINEAR_ADDRESS_BREAKER linear_address;
  FN_ENTRY();
  linear_address.address = address;

  assert(address_space->pde_base);

  FN_LEAVE();
  return &address_space->pde_base[linear_address.u.PDE_IDX];
}


/** @function  vmm_share_physical_range
 *  @brief     Given a range installed into both the source and
 *             the destination address spaces, this function
 *             backs the dst range with same physical page mappings as the src range.
 *  @note      Range must be installed first.
 *             "Install": This points to having PDE and PTE entries for the vm range
 *  @param     vm_dst - pointer to destination task's VM
 *  @param     vm_src - pointer to source task's VM
 *  @param     range  - pointer to the range that must be shared
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE vmm_share_physical_range(struct task_vm *vm_dst,
				       struct task_vm *vm_src,
				       vm_range       *range)
{
  uint32_t linear_address;
  PTE      *src_pte;
  PTE      *dst_pte;
  for(linear_address = (uint32_t)range->start;
      linear_address < (uint32_t)(range->start + range->len);
      linear_address += PAGE_SIZE) {
    //- Get the source and destination PTE for the linear address -//
    src_pte = vmm_get_pte(vm_src,linear_address);
    dst_pte = vmm_get_pte(vm_dst,linear_address);

    if(!src_pte || !dst_pte) {
      panic("range has to be installed first in both address spaces");
    }

    //- make dst share the physical page as source -//
    vmm_getref_user_page(src_pte->ADDRESS);
    dst_pte->ADDRESS = src_pte->ADDRESS;
  }
  return KERN_SUCCESS;
}


/** @function  vmm_back_all_user_ranges
 *  @brief     This function will back the task's user VM ranges
 *             by allocating frames and setting PTE/PDE entries
 *  @param     vm - pointer to task's VM
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE vmm_back_all_user_ranges(struct task_vm *vm) {
  vm_range *this_range;
  uint32_t linear_address;
  PTE *pte;
  PFN pfn;

  Q_FOREACH( this_range , &vm->vm_ranges_head ,vm_range_next ) {
    //- Skip the kernel range -//
    if(this_range == &vm->vm_range_kernel)
      continue;

    //-- install pages --//
    for(linear_address = (uint32_t)this_range->start;
	linear_address < (uint32_t)(this_range->start + this_range->len);
	linear_address += PAGE_SIZE) {

      pte = vmm_get_pte(vm,linear_address);
      assert(pte);
      if(KERN_SUCCESS != vmm_get_free_user_pages(&pfn))
	return KERN_NO_MEM;

      pte->PRESENT = 1;
      pte->ADDRESS = pfn;
    }//--end 1 range --//
  }//-- End all range

  return KERN_SUCCESS;
}


/** @function  vmm_unback_all_user_ranges
 *  @brief     This function will unback the task's user VM ranges
 *             by deallocating frames and unsetting PTE/PDE entries
 *  @param     vm - pointer to task's VM
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE vmm_unback_all_user_ranges(struct task_vm *vm)
{
  vm_range *this_range;
  uint32_t linear_address;
  PTE *pte;

  Q_FOREACH( this_range , &vm->vm_ranges_head ,vm_range_next ) {
    //- Skip the kernel range -//
    if(this_range == &vm->vm_range_kernel)
      continue;

    //-- unmap pages --//
    for(linear_address = (uint32_t)this_range->start;
	linear_address < (uint32_t)(this_range->start + this_range->len);
	linear_address += PAGE_SIZE) {
      pte = vmm_get_pte(vm,linear_address);
      assert(pte);

      // -- we have this check because vm ranges abstraction can overlap -- //
      // -- so some pages might be dereferenced earlier -- //
      // --and thier PTE->address set to 0 -- //
      if(pte->ADDRESS)
	vmm_putref_user_page(pte->ADDRESS);

      pte->PRESENT = 0;
      pte->ADDRESS = 0;
    }//--end 1 range --//
  }//-- End all range

  return KERN_SUCCESS;
}


/** @function  vmm_set_range_attr
 *  @brief     This function will set the supplied attributes
 *             to all user pages (in PDE/PTEs)
 *  @param     vm    - pointer to task's VM
 *  @param     range - pointer to vm_range whose attr must be set
 *  @param     attrs - the attribute values that are to be set
 *  @return    KERN_SUCCESS on completion
 */

KERN_RET_CODE vmm_set_range_attr(struct task_vm  *vm,
				 struct vm_range *range,
				 PDE    attrs)
{
  unsigned long linear_address;
  PDE *pde;
  PTE *pte;
  PTE temp;

  attrs.ADDRESS = -1;

  for(linear_address = range->start;
      linear_address < range->start + range->len;
      linear_address += PAGE_SIZE) {

    pte = vmm_get_pte(vm,linear_address);
    assert(pte);
    temp.ADDRESS = pte->ADDRESS;
    *pte = attrs;
    pte->ADDRESS = temp.ADDRESS;



    pde = vmm_get_pde(vm,linear_address);
    assert(pde);
    temp.ADDRESS = pde->ADDRESS;
    *pde = attrs;
    pde->ADDRESS = temp.ADDRESS;
  }//--end 1 range --//

  return KERN_SUCCESS;
}


/** @function  vmm_copy_vmranges_struct
 *  @brief     This function wipes out all the user ranges in the destination VM
 *             and then copies over all the vm ranges from source to destination VM
 *             It also copies the ELF references across VMs
 *  @param     vm_dst - pointer to destination task's VM
 *  @param     vm_src - pointer to source task's VM
 *  @return    KERN_SUCCESS on completion; KERN_NO_MEM if mem alloc fails
 */

KERN_RET_CODE vmm_copy_vmranges_struct(struct task_vm *vm_dst,
				       struct task_vm *vm_src)
{
  vm_range *vmrange_ptr,*vmrange_ptr_save,*new_range;

  //- free all old ranges  -//
  Q_FOREACH_DEL_SAFE(vmrange_ptr,
		     &vm_dst->vm_ranges_head,
		     vm_range_next,
		     vmrange_ptr_save)  {
    if(vmrange_ptr == &vm_dst->vm_range_kernel)
      continue;

    Q_REMOVE(&vm_dst->vm_ranges_head,
	     vmrange_ptr,
	     vm_range_next);
    free(vmrange_ptr);
  }

  //-- allocated and copy over the ranges  --//
  Q_FOREACH( vmrange_ptr , &vm_src->vm_ranges_head , vm_range_next )  {
    if(vmrange_ptr == &vm_src->vm_range_kernel)
      continue;

    //-- allocate the range structure --//
    new_range = malloc(sizeof(*new_range));
    if(!new_range)
      return KERN_NO_MEM;

    //-- Insert the range on to the list of available ranges --//
    new_range->start = vmrange_ptr->start;
    new_range->len   = vmrange_ptr->len;
    Q_INIT_ELEM( new_range , vm_range_next );
    Q_INSERT_TAIL( &vm_dst->vm_ranges_head ,
		    new_range ,
		    vm_range_next );

  }

  //-- copy over references to ranges --//
  vm_dst->vm_text_start = vm_src->vm_text_start;
  vm_dst->vm_text_len   = vm_src->vm_text_len;

  vm_dst->vm_data_start = vm_src->vm_data_start;
  vm_dst->vm_data_len   = vm_src->vm_data_len;

  vm_dst->vm_rdata_start = vm_src->vm_rdata_start;
  vm_dst->vm_rdata_len   = vm_src->vm_rdata_len;

  vm_dst->vm_stack_start = vm_src->vm_stack_start;
  vm_dst->vm_stack_len   = vm_src->vm_stack_len;

  return KERN_SUCCESS;
}


/** @function  vmm_get_range
 *  @brief     This function checks whether the supplied user address
 *             is already a part of the user VM or not
 *  @param     vm      - pointer to the task's VM
 *  @param     address - user address in question
 *  @return    pointer to vm_range if address exists in range; NULL if not
 */

vm_range *vmm_get_range( struct task_vm *vm , char *address ) {

  vm_range *vmrange_ptr;
  FN_ENTRY();

  Q_FOREACH( vmrange_ptr , &vm->vm_ranges_head , vm_range_next )  {

    if(vmrange_ptr == &vm->vm_range_kernel)
      continue;

    if( ( address >= (char *)(vmrange_ptr->start) )
	&&( address < (char *)(vmrange_ptr->start + vmrange_ptr->len) ) ) {
      FN_LEAVE();
      return vmrange_ptr;
    }
  }

  FN_LEAVE();
  return NULL;
}

/** @function  vmm_is_range_present
 *  @brief     This function checks whether the supplied user range
 *             is already a part of the user VM or not
 *  @param     vm        - pointer to the task's VM
 *  @param     base_addr - user address in question
 *  @param     len       - length of range starting at base_addr
 *  @return    KERN_SUCCESS if the range is present; KERN err code if absent
 */

KERN_RET_CODE vmm_is_range_present( struct task_vm *vm , void *base_addr , int len ) {
  vm_range *vmrange_ptr;
  char *next_addr = (char *)base_addr;
  int offset;

  FN_ENTRY();

  while(1) {
    vmrange_ptr = vmm_get_range( vm , next_addr );
    if( NULL == vmrange_ptr ) /* -- outside vm_range -- */ {
      return KERN_ERROR_ADDRESS_NOT_PRESENT;
    }

    offset = (int)( vmrange_ptr->start + vmrange_ptr->len  - (unsigned long) next_addr);

    if( len <= offset ) /* -- within single vm_range -- */ {
      return KERN_SUCCESS;
    }

    next_addr = next_addr + offset;

  }

  FN_LEAVE();
  return KERN_ERROR_UNIMPLEMENTED;
}

int  vmm_is_address_ro( struct task_vm *vm , void *base_addr ) {
  unsigned long local_base_addr = (unsigned long) base_addr;

  if(local_base_addr >= vm->vm_text_start &&
     local_base_addr < (vm->vm_text_start + vm->vm_text_len))
    return 1;

  if(local_base_addr >= vm->vm_rdata_start &&
     local_base_addr < (vm->vm_rdata_start + vm->vm_rdata_len))
    return 1;

  return 0;
}
