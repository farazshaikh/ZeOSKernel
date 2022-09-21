/** @file     i386systemregs.c
 *  @brief    This file contains functions for handling i386 register internals
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 *
 *  IMPLEMENATION NOTE:
 *       ISR template Code Generation:
 *       -Having a driver writer write a assembly wrapper for installing and ISR
 *       is inconvinient. All off these code wrappers have the same code except
 *       for the address of the higher level 'C' function.
 *
 *       -To avoid the need of having to write ASM code for every ISR entry we AUTO
 *       generate code wrappers from base ISR template and then patch-in the 'C'
 *       function call address into the newly generated code block. This newly created
 *       code block with the patched in C call address is then saved in the IDT.
 *
 *       -Code blocks generated come from the kernel memory allocator. Its reasonable
 *       to assume in the 410 land that memory blocks from malloc can be jumped into
 *       for code execution. Implicity places constraint that KERNEL_CS == KERNEL_DS
 *
 *       -CALL instruction patching is of type intersegment 32 bit relative address.
 *        so no long jumps (hardly anybody does them and gas has cryptic ways of
 *        specifying segment override prefixes for CALL)
 *
 *        IMPLEMENTATION IDEA IS COPIED FROM WINDOWS DOCUMENTATION.
 */

#include <stddef.h>
#include <stdio.h>
#include <kern_common.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <seg.h>
#include <asm.h>

#include "i386lib/i386systemregs.h"



char *pisr_wrapper_template;            //- Don't need so many globals   -//
char *piw_patchoffset;                  //- Helps understand deviant art pieces-//
char *piw_end;
char *piw_next_instroffset;

int   patch_offset;
int   next_instroffset;


/** @function  offsets_fixup
 *  @brief     This function fixes up offsets required
 *             for code patching in the ISR template
 *  @note      three parameter address comes as externs
 *             from ASM file that declares the isr template
 *             [isr_wrapper_template/iw_end/iw_patchoffset/iw_next_instroffset]
 *  @param     none
 *  @return    KERN_SUCCESS on completion
 */

static KERN_RET_CODE offsets_fixup(void)
{
  FN_ENTRY();

  //-- We defined code in asm .data section --//
  pisr_wrapper_template = &isr_wrapper_template;
  piw_patchoffset = &iw_patchoffset;
  piw_end         = &iw_end;
  piw_next_instroffset = &iw_next_instroffset;

  //-- Get absolute values for patch offssets -//
  patch_offset     = piw_patchoffset - pisr_wrapper_template;
  next_instroffset = piw_next_instroffset - pisr_wrapper_template;

  FN_LEAVE();
  return KERN_SUCCESS;
}



/** @function  patch_code_block
 *  @brief     This function patches a ISR template code block
 *             with the call offset of fn_addr
 *  @param     code_block address of the code block to be patched
 *  @param     fn_addr the function address to be patched
 *  @return    KERN_SUCCESS on completion
 */

static KERN_RET_CODE patch_code_block(
				      char *code_block,
				      char *fn_addr)
{
  int rel32offset; //-- For encoding rel32offset from next instr -//
  char *next_instraddr;

  FN_ENTRY();
  //-- offsets_fixup base ISR wrapper template --//
  offsets_fixup();

  //-- get the offsets correct --//
  next_instraddr = code_block + next_instroffset;
  rel32offset    = fn_addr - next_instraddr;

  //- patch up rel32 address -//
  *(char **)(code_block + patch_offset +  1) = (char *)rel32offset;

  FN_LEAVE();
  return KERN_SUCCESS;
}



/** @function  i386_set_idt_entry
 *  @brief     formats the entry in the IDT to materialize ISR installation
 *  @param     idt_base pointer to the base of the IDT
 *  @param     code_segment_sel code segment selector for this ISR
 *  @param     idt_offset IDT offset to be used for insstallation
 *  @param     type of gate to be installed one of i386_GATE_TYPE_[TASK/INTR/TRAP]
 *  @return    KERN_SUCCESS on successful installation of IDT entry
 *             Appropriate failure code or panic message on failure
 */

KERN_RET_CODE i386_set_idt_entry(
				 char *idt_base,
				 short code_segment_sel,
				 char *code_offset,
				 char  idt_offset,
				 i386_IDT_GATE_TYPE gatetype,
				 int   DPL
				 )
{
  KERN_RET_CODE ret = KERN_ERROR_UNIMPLEMENTED; //-- Will be completed by DEC -//
  FN_ENTRY();
  union IDT_OFFSET_BREAKER idt_offset_breaker;
  idt_offset_breaker.offset = (int) code_offset;


  if( i386_GATE_TYPE_TRAP == gatetype ) {

    //-- Install a trap Gate --//
    i386_TRAP_GATE_DESC *pI386_trap_gate_desc = NULL;
    C_ASSERT( IDT_ENTRY_SIZE == sizeof( *pI386_trap_gate_desc ) );
    pI386_trap_gate_desc = (i386_TRAP_GATE_DESC *)idt_base  + idt_offset;

    //-- Init fields --//
    pI386_trap_gate_desc->OFFSET_LOWER = idt_offset_breaker.u.offset_lower;
    pI386_trap_gate_desc->SEGMENT_SEL  = code_segment_sel;

    pI386_trap_gate_desc->UNDEF  = UNDEF_BITS;
    pI386_trap_gate_desc->ZEROS3 = ZEROS_BITS;
    pI386_trap_gate_desc->TYPE   = i386_GATE_TYPE_TRAP;
    pI386_trap_gate_desc->SIZE   = (char) ONES_BITS;
    pI386_trap_gate_desc->ZEROS1 = ZEROS_BITS;
    pI386_trap_gate_desc->DPL    = DPL;
    pI386_trap_gate_desc->PRESENT= (char) ONES_BITS;

    pI386_trap_gate_desc->OFFSET_UPPER = idt_offset_breaker.u.offset_upper;
    ret = KERN_SUCCESS;
  }else if ( i386_GATE_TYPE_INTR == gatetype ) {

    //-- Install a interrupt Gate -//
    i386_INTR_GATE_DESC *pI386_intr_gate_desc = NULL;
    C_ASSERT( IDT_ENTRY_SIZE == sizeof( *pI386_intr_gate_desc ) );
    pI386_intr_gate_desc = (i386_INTR_GATE_DESC *)idt_base  + idt_offset;

    //-- Init fields --//
    pI386_intr_gate_desc->OFFSET_LOWER = idt_offset_breaker.u.offset_lower;
    pI386_intr_gate_desc->SEGMENT_SEL  = code_segment_sel;

    pI386_intr_gate_desc->UNDEF  = UNDEF_BITS;
    pI386_intr_gate_desc->ZEROS3 = ZEROS_BITS;
    pI386_intr_gate_desc->TYPE   = i386_GATE_TYPE_INTR;
    pI386_intr_gate_desc->SIZE   = (char) ONES_BITS;
    pI386_intr_gate_desc->ZEROS1 = ZEROS_BITS;
    pI386_intr_gate_desc->DPL    = DPL;
    pI386_intr_gate_desc->PRESENT= (char) ONES_BITS;

    pI386_intr_gate_desc->OFFSET_UPPER = idt_offset_breaker.u.offset_upper;
    ret = KERN_SUCCESS;

  }else if ( i386_GATE_TYPE_TASK == gatetype ) {

    //-- Install a task Gate -//
    assert(0);

  }else {
    //-- Panic --//
    panic("PANIC: This type of ISR cannot be installed %d" , gatetype);
  }

  FN_LEAVE();
  return ret;
}


/** @function  i386_install_isr
 *  @brief     Installs an ISR into i386 idt with a callback in pisr 'c code fn'
 *  @param     pisr address of the C function to handle the ISR
 *  @param     idt_offset the offset into the IDT for installing this ISR
 *  @param     type of gate to be installed one of i386_GATE_TYPE_[TASK/INTR/TRAP]
 *  @param     DPL discriptor privilege level for the ISR handler
 *  @return    KERN_SUCCESS on success, appropriate failure code on error
 */

KERN_RET_CODE i386_install_isr(
			       pI386_isr_fn       pisr,
			       char               idt_offset,
			       i386_IDT_GATE_TYPE gatetype,
			       int                DPL
			       )
{
  KERN_RET_CODE ret;
  char *pISRWrapperCode = NULL; //-- Will be generated by malloc & code copy -//
  FN_ENTRY();
  DEBUG_PRINT("Installing %p at IDT offset",pisr,idt_offset);

  //-- fixup the offsets upfront --//
  offsets_fixup();

  //-- Allocate Code block for the new ISR wrapper --//
  pISRWrapperCode = malloc( piw_end - pisr_wrapper_template );
  if( NULL == pISRWrapperCode ) {
    DUMP( "cannot allocate memory for installing ISR code wrapper" );
    return KERN_NO_MEM;
  }


  //- copy off the ISR template code into allocated code block -//
  memcpy( pISRWrapperCode , pisr_wrapper_template , piw_end - pisr_wrapper_template );


  //- patch the C handler offset in code block -//
  //- a.k.a Plant Bubble gum prince in choclate land -//
  ret = patch_code_block( pISRWrapperCode ,(char *) pisr );
  assert( KERN_SUCCESS == ret );


  //-- Install ISR entry into IDT now --//
  ret = i386_set_idt_entry((char *) idt_base(),
			   SEGSEL_KERNEL_CS,
			   pISRWrapperCode,
			   idt_offset,
			   gatetype,
			   DPL
			   );
  assert( KERN_SUCCESS == ret );


  DEBUG_PRINT("bubble gum prince glued in choclate land");
  FN_LEAVE();
  return ret;
}
