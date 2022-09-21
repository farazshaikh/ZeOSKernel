/** @file     i386systemregs.h
 *  @brief    This file contains the interfaces for the handling i386 processor
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _I386_SYSTEM_REGS
#define _I386_SYSTEM_REGS

#include <kern_common.h>

#define IDT_ENTRY_SIZE               8       //- Compile time asserts -//
#define ONES_BITS                    0xffffffff
#define ZEROS_BITS                   0x0
#define UNDEF_BITS                   ZEROS_BITS


/** @constant  i386_PLX
 *  @brief privileges levels of the i386 processor
 */

#define i386_PL0                     0
#define i386_PL1                     1
#define i386_PL2                     2
#define i386_PL3                     3




typedef int IDT_BASE_DS;
#define IDT_GATE_BIT_FIELD(gatetype,fieldname) \
    IDT_BASE_DS fieldname : gatetype##_##fieldname##_BITS

#define IDT_GATE_ZEROS3_BITS         3
#define IDT_GATE_ZEROS1_BITS         1
#define IDT_GATE_TYPE_BITS           3      //- Type of Gate Bits      -//
#define IDT_GATE_SIZE_BITS           1      //- D Bit in all IDT Gates -//
#define IDT_GATE_DPL_BITS            2      //- DPL bits               -//
#define IDT_GATE_PRESENT_BITS        1      //- P Bit in all IDT Gates -//
#define IDT_GATE_OFFSET_UPPER_BITS   16     //- Offset 31...16 bits    -//
#define IDT_GATE_OFFSET_LOWER_BITS   16     //- Offset 0...16 bits     -//
#define IDT_GATE_SEGMENT_SEL_BITS    16     //- Seg Sel bits 0-16      -//


typedef enum  {
  i386_GATE_TYPE_TASK = 0x05,
  i386_GATE_TYPE_INTR = 0x06,
  i386_GATE_TYPE_TRAP = 0x07,
  i386_GATE_TYPE_MAX  = 0xffffff
}i386_IDT_GATE_TYPE;




//------------------------------------------------------------------------------
// TRAP GATE
//------------------------------------------------------------------------------
/** @constant  TRAP_GATE_XXX_BITS
 *  @brief sizes on individual fields in the trap gate descriptor
 *  @note most are derived from common field sizes of the IDT descriptor
 *
 */

#define TRAP_GATE_UNDEF_BITS         5
#define TRAP_GATE_ZEROS3_BITS        IDT_GATE_ZEROS3_BITS
#define TRAP_GATE_TYPE_BITS          IDT_GATE_TYPE_BITS
#define TRAP_GATE_SIZE_BITS          IDT_GATE_SIZE_BITS
#define TRAP_GATE_ZEROS1_BITS        IDT_GATE_ZEROS1_BITS
#define TRAP_GATE_DPL_BITS           IDT_GATE_DPL_BITS
#define TRAP_GATE_PRESENT_BITS       IDT_GATE_PRESENT_BITS
#define TRAP_GATE_OFFSET_UPPER_BITS  IDT_GATE_OFFSET_UPPER_BITS
#define TRAP_GATE_OFFSET_LOWER_BITS  IDT_GATE_OFFSET_LOWER_BITS
#define TRAP_GATE_SEGMENT_SEL_BITS   IDT_GATE_SEGMENT_SEL_BITS


/** @typedef  i386_TRAP_GATE_DESC
 *  @brief    'C' style accessing pointer def for the trap gate descriptor
 *  @NOTE     changing size or order will cause compile time asserts
 */

struct _i386_TRAP_GATE_DESC  {
  //-- First 32 bits --//
  IDT_GATE_BIT_FIELD(TRAP_GATE,OFFSET_LOWER);
  IDT_GATE_BIT_FIELD(TRAP_GATE,SEGMENT_SEL);

  //- Second 32 bits -//
  IDT_GATE_BIT_FIELD(TRAP_GATE,UNDEF);
  IDT_GATE_BIT_FIELD(TRAP_GATE,ZEROS3);
  IDT_GATE_BIT_FIELD(TRAP_GATE,TYPE);
  IDT_GATE_BIT_FIELD(TRAP_GATE,SIZE);
  IDT_GATE_BIT_FIELD(TRAP_GATE,ZEROS1);
  IDT_GATE_BIT_FIELD(TRAP_GATE,DPL);
  IDT_GATE_BIT_FIELD(TRAP_GATE,PRESENT);

  IDT_GATE_BIT_FIELD(TRAP_GATE,OFFSET_UPPER);
}PACKED;
typedef struct _i386_TRAP_GATE_DESC i386_TRAP_GATE_DESC;



//------------------------------------------------------------------------------
// INTR GATE
//------------------------------------------------------------------------------
/** @constant  INTR_GATE_XXX_BITS
 *  @brief sizes on individual fields in the interupt gate descriptor
 *  @note most are derived from common field sizes of the IDT descriptor
 *
 */

#define INTR_GATE_UNDEF_BITS         5
#define INTR_GATE_ZEROS3_BITS        IDT_GATE_ZEROS3_BITS
#define INTR_GATE_TYPE_BITS          IDT_GATE_TYPE_BITS
#define INTR_GATE_SIZE_BITS          IDT_GATE_SIZE_BITS
#define INTR_GATE_ZEROS1_BITS        IDT_GATE_ZEROS1_BITS
#define INTR_GATE_DPL_BITS           IDT_GATE_DPL_BITS
#define INTR_GATE_PRESENT_BITS       IDT_GATE_PRESENT_BITS
#define INTR_GATE_OFFSET_UPPER_BITS  IDT_GATE_OFFSET_UPPER_BITS
#define INTR_GATE_OFFSET_LOWER_BITS  IDT_GATE_OFFSET_LOWER_BITS
#define INTR_GATE_SEGMENT_SEL_BITS   IDT_GATE_SEGMENT_SEL_BITS


/** @typedef  i386_INTR_GATE_DESC
 *  @brief    'C' style accessing pointer def for the intr gate descriptor
 *  @NOTE     changing size or order will cause compile time asserts
 */

struct _i386_INTR_GATE_DESC  {
  //-- First 32 bits --//
  IDT_GATE_BIT_FIELD(INTR_GATE,OFFSET_LOWER);
  IDT_GATE_BIT_FIELD(INTR_GATE,SEGMENT_SEL);

  //- Second 32 bits -//
  IDT_GATE_BIT_FIELD(INTR_GATE,UNDEF);
  IDT_GATE_BIT_FIELD(INTR_GATE,ZEROS3);
  IDT_GATE_BIT_FIELD(INTR_GATE,TYPE);
  IDT_GATE_BIT_FIELD(INTR_GATE,SIZE);
  IDT_GATE_BIT_FIELD(INTR_GATE,ZEROS1);
  IDT_GATE_BIT_FIELD(INTR_GATE,DPL);
  IDT_GATE_BIT_FIELD(INTR_GATE,PRESENT);

  IDT_GATE_BIT_FIELD(INTR_GATE,OFFSET_UPPER);
}PACKED;
typedef struct _i386_INTR_GATE_DESC i386_INTR_GATE_DESC;


/** @typedef  IDT_OFFSET_BREAKER
 *  @brief    Convinient way to access lower and upper halves of ISR code offsets
 */
union IDT_OFFSET_BREAKER {
  int offset;                         //-- 32 bit ISR code offset     --//
  struct {
    unsigned short offset_lower;      //-- offset lower half 16 bits  --//
    unsigned short offset_upper;      //-- offfset upper half 16 bits --//
  }u;
} PACKED;


//------------------------------------------------------------------------------
// Functions to be exported see definitions for documentation
//------------------------------------------------------------------------------

typedef void(*pI386_isr_fn)(void);
KERN_RET_CODE i386_install_isr(pI386_isr_fn,char,i386_IDT_GATE_TYPE,int);
KERN_RET_CODE i386_set_idt_entry(
				 char *idt_base,
				 short code_segment_sel,
				 char *code_offset,
				 char  idt_offset,
				 i386_IDT_GATE_TYPE gatetype,
				 int   DPL
				 );

//------------------------------------------------------------------------------
// definition exported from ASM code
//------------------------------------------------------------------------------

extern char isr_wrapper_template;
extern char iw_patchoffset;
extern char iw_end;
extern char iw_next_instroffset;


//------------------------------------------------------------------------------
// PTE PDE PDBR
//------------------------------------------------------------------------------

typedef char * PDBR;
typedef uint32_t PTE_BASE_DS;
#define PTE_ENTRY_SIZE          4  //-- Compile time asserts --//

#define PTE_PRESENT_BITS        1
#define PTE_RW_BITS             1
#define PTE_US_BITS             1
#define PTE_WT_BITS             1
#define PTE_CACHE_DISABLED_BITS 1
#define PTE_ACCESSED_BITS       1
#define PTE_DIRTY_BITS          1
#define PTE__PAGE_SIZE_BITS      1
#define PTE_GLOBAL_BITS         1
#define PTE_AVAIL_BITS          3
#define PTE_ADDRESS_BITS        20

#define PTE_BIT_FIELD(PTE_PRESET,fieldname) \
    PTE_BASE_DS fieldname : PTE_PRESET##_##fieldname##_BITS

struct  _PTE {
  PTE_BIT_FIELD(PTE,PRESENT);
  PTE_BIT_FIELD(PTE,RW);
  PTE_BIT_FIELD(PTE,US);
  PTE_BIT_FIELD(PTE,WT);
  PTE_BIT_FIELD(PTE,CACHE_DISABLED);
  PTE_BIT_FIELD(PTE,ACCESSED);
  PTE_BIT_FIELD(PTE,DIRTY);
  PTE_BIT_FIELD(PTE,_PAGE_SIZE);
  PTE_BIT_FIELD(PTE,GLOBAL);
  PTE_BIT_FIELD(PTE,AVAIL);
  PTE_BIT_FIELD(PTE,ADDRESS);
}PACKED;
typedef struct _PTE PTE,*PPTE,PDE,*PPDE;

#define PAGING_PAGE_OFFSET_BITS 12
#define PAGING_PTE_INDX_BITS    10
#define PAGING_PDE_INDX_BITS    10
#define PAGE_MASK               0xfff

/** @typedef  LINEAR_ADDRESS_BREAKER
 *  @brief    Convinient way to access PDE/PTE idx from linear address
 */
union _LINEAR_ADDRESS_BREAKER {
  PTE_BASE_DS address;                                //-- 32 bit linear address --//
  struct {
    PTE_BASE_DS PAGE_OFFSET:PAGING_PAGE_OFFSET_BITS; //- offset into page --//
    PTE_BASE_DS PTE_IDX:PAGING_PTE_INDX_BITS;        //-- offset into PTE --//
    PTE_BASE_DS PDE_IDX:PAGING_PDE_INDX_BITS;        //-- offset into PDE --//
  }u;
} PACKED;
typedef union _LINEAR_ADDRESS_BREAKER LINEAR_ADDRESS_BREAKER;

//------------------------------------------------------------------------------
// CONTROL REGISTERS
//------------------------------------------------------------------------------
typedef int CR_BASE_DS ;
#define CR_REG_SIZE          4                    //-- Compile time asserts --//
#define CR_BIT_FIELD(CR_PRESET,fieldname) \
    CR_BASE_DS fieldname : CR_PRESET##_##fieldname##_BITS

//-- CR0 --//
#define CR0_PE_BITS 1
#define CR0_MP_BITS 1
#define CR0_EM_BITS 1
#define CR0_TS_BITS 1
#define CR0_ET_BITS 1
#define CR0_NE_BITS 1
#define CR0_UNDEF10_BITS 10
#define CR0_WP_BITS 1
#define CR0_UNDEF1_BITS 1
#define CR0_AM_BITS 1
#define CR0_UNDEF2_10_BITS 10
#define CR0_NW_BITS 1
#define CR0_CD_BITS 1
#define CR0_PG_BITS 1
union _CR0  {
  CR_BASE_DS cr_val;
  struct {
    CR_BIT_FIELD(CR0,PE);
    CR_BIT_FIELD(CR0,MP);
    CR_BIT_FIELD(CR0,EM);
    CR_BIT_FIELD(CR0,TS);
    CR_BIT_FIELD(CR0,ET);
    CR_BIT_FIELD(CR0,NE);
    CR_BIT_FIELD(CR0,UNDEF10);
    CR_BIT_FIELD(CR0,WP);
    CR_BIT_FIELD(CR0,UNDEF1);
    CR_BIT_FIELD(CR0,AM);
    CR_BIT_FIELD(CR0,UNDEF2_10);
    CR_BIT_FIELD(CR0,NW);
    CR_BIT_FIELD(CR0,CD);
    CR_BIT_FIELD(CR0,PG);
  }u;
}PACKED;
typedef union _CR0 CR0;

//-- CR1 --//
struct _CR1  {
  CR_BASE_DS cr_val;
}PACKED;
typedef struct _CR1 CR1;

//-- CR2 --//
struct _CR2  {
  CR_BASE_DS page_fault_linear_address;
}PACKED;
typedef struct _CR2 CR2;


//-- CR3 --//
#define CR3_UNDEF3_BITS 3
#define CR3_PWT_BITS 1
#define CR3_PCD_BITS 1
#define CR3_UNDEF7_BITS 7
#define CR3_PDBASE_BITS 20
union _CR3  {
  CR_BASE_DS cr_val;
  struct {
    CR_BIT_FIELD(CR3,UNDEF3);
    CR_BIT_FIELD(CR3,PWT);
    CR_BIT_FIELD(CR3,PCD);
    CR_BIT_FIELD(CR3,UNDEF7);
    CR_BIT_FIELD(CR3,PDBASE);
  }u;
}PACKED;
typedef union _CR3 CR3;

//-- CR4 --//
#define CR4_VME_BITS      1
#define CR4_PVI_BITS      1
#define CR4_TSD_BITS      1
#define CR4_DE_BITS       1
#define CR4_PSE_BITS      1
#define CR4_PAE_BITS      1
#define CR4_MCE_BITS      1
#define CR4_PGE_BITS      1
#define CR4_PCE_BITS      1
#define CR4_FXSR_BITS     1
#define CR4_MMEXCPT_BITS  1
#define CR4_RESERVED_BITS 21
union _CR4  {
  CR_BASE_DS cr_val;
  struct {
    CR_BIT_FIELD(CR4,VME);
    CR_BIT_FIELD(CR4,PVI);
    CR_BIT_FIELD(CR4,TSD);
    CR_BIT_FIELD(CR4,DE);
    CR_BIT_FIELD(CR4,PSE);
    CR_BIT_FIELD(CR4,PAE);
    CR_BIT_FIELD(CR4,MCE);
    CR_BIT_FIELD(CR4,PGE);
    CR_BIT_FIELD(CR4,PCE);
    CR_BIT_FIELD(CR4,FXSR);
    CR_BIT_FIELD(CR4,MMEXCPT);
    CR_BIT_FIELD(CR4,RESERVED);
  }u;
}PACKED;
typedef union _CR4 CR4;

typedef uint32_t STACK_ELT;
struct _IRET_FRAME {
  STACK_ELT eip;
  STACK_ELT cs;
  STACK_ELT eflags;
  STACK_ELT esp;
  STACK_ELT ss;
}PACKED;
typedef struct _IRET_FRAME IRET_FRAME;

// Fault numbers //
#define FAULT_DE      0
#define FAULT_DB      1
#define FAULT_NMI     2
#define FAULT_BP      3
#define FAULT_OF      4
#define FAULT_BR      5
#define FAULT_UD      6
#define FAULT_NM      7
#define FAULT_DF      8
#define FAULT_CSO     9
#define FAULT_TS      10
#define FAULT_NP      11
#define FAULT_SS      12
#define FAULT_GP      13
#define FAULT_PF      14
#define FAULT_RESERVED 15
#define FAULT_MF      16
#define FAULT_AC      17
#define FAULT_MC      18
#define FAULT_XF      19


#endif // _I386_SYSTEM_REGS
