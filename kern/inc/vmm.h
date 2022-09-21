/** @file     vmm.h
 *  @brief    This file defines the virtual memory management primitives
 *            structures and types, and function prototypes that are exported
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _VMM_H
#define _VMM_H
#include <kern_common.h>
#include <x86/page.h>

#define KERNEL_PAGES_NR     (USER_MEM_START / PAGE_SIZE)
#define KTHREAD_KSTACK_PAGES 2
#define INITIAL_PDE_PAGES    1
#define PTE_PER_PAGE         (PAGE_SIZE/sizeof(PTE))
typedef unsigned int PFN;


Q_NEW_HEAD( vm_ranges_head , vm_range );

// -- refcount used in COW setup -- //
struct _m_page {
  volatile int refcount;
}PACKED;

typedef struct _m_page m_page;

// -- each vm range node contains the range of memory used by a task's VM -- //
// -- growing VM adds nodes to VQ implementation with info on range accessible -- //   
typedef struct vm_range {
  Q_NEW_LINK( vm_range ) vm_range_next;
  unsigned long start;
  unsigned long len; 
}vm_range; 

// -- the actual VM manager struct that is a part of each task -- //

struct task_vm {
  vm_ranges_head vm_ranges_head;
  vm_range       vm_range_kernel;  //0-USER_MEM_START

  //- Initial exec ranges --//
  unsigned long vm_text_start; 
  unsigned long vm_text_len; 

  unsigned long vm_data_start;
  unsigned long vm_data_len;

  unsigned long vm_rdata_start;
  unsigned long vm_rdata_len; 


  unsigned long vm_stack_start; 
  unsigned long vm_stack_len; 

  //- Initial setup -//
  PDE   *pde_base;
  char  *taskmem;
  int    totalTaskAllocation;
}; 


// -- VMM Manager for the kernel -- //
typedef struct _kern_vmm { 
  m_page *m_pages;
  int nr_physical_pages; 
  int nr_free_pages; 
  PFN next_free_page;
}kern_vmm; 

typedef struct ktask ktask;


//- USER PAGE MANAGEMENT -//
KERN_RET_CODE vmm_get_free_user_pages(PFN *pfn);
void vmm_getref_user_page(PFN pfn);
void vmm_putref_user_page(PFN pfn);



//- KERN TASK ALLOC and FREE -//
KERN_RET_CODE vmm_init_task_vm(ktask *parentTask,ktask **ppKtask);
void          vmm_free_all_vma(struct task_vm *vm_dst);
void          vmm_free_task_vm(ktask *pTask);

void          vmm_free_task_vm_top(ktask *pTask);
void          vmm_free_task_vm_bottom(ktask *pTask);


//- SUBSYSTEM INIT -//
KERN_RET_CODE vmm_init(void); 


//-- Address space manipulations --//
//-- Ranges need to start and end at page boundaries --//
KERN_RET_CODE vmm_install_range(struct task_vm *,vm_range *);
KERN_RET_CODE vmm_uninstall_range(struct task_vm *,vm_range*);
KERN_RET_CODE vmm_free_user_ptes(struct task_vm*);



//-- we don't have an corresponding free kernel mod pte --//
//-- because we never do that                           --//
KERN_RET_CODE vmm_copy_user_ptes(struct task_vm *dst,
				struct task_vm *src);



//-- Get the pte for an address --//
PTE* vmm_get_pte(struct task_vm *vm,uint32_t address);
PDE* vmm_get_pde(struct task_vm *vm,uint32_t address);

//-- Kern VMM back all User ranges --//
KERN_RET_CODE vmm_back_all_user_ranges(struct task_vm *vm);
KERN_RET_CODE vmm_unback_all_user_ranges(struct task_vm *vm);



//- RANGE Attribute Setting -//
KERN_RET_CODE vmm_set_range_attr(struct task_vm  *vm,
				 struct vm_range *range,
				 PDE    attrs);

//- Copy all vm ranges struct from one struct to other -//
KERN_RET_CODE vmm_copy_vmranges_struct(struct task_vm *vm_dst,
				       struct task_vm *vm_src);


KERN_RET_CODE vmm_share_physical_range(struct task_vm *vm_dst,
				       struct task_vm *vm_src,
				       vm_range       *range);

// --Address range checking functions --//
vm_range *vmm_get_range( struct task_vm *vm , char *address );
KERN_RET_CODE vmm_is_range_present( struct task_vm *vm , void *base_addr , int len );
int  vmm_is_address_ro( struct task_vm *vm , void *base_addr );
#endif // _VMM_H


