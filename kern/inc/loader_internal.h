/** @file     loader_internal.h
 *  @brief    This file exports the prototype used to actually load 
 *            the ELF file contents into a task
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _LOADER_INTERNAL_H
#define _LOADER_INTERNAL_H
#include <kern_common.h>
#include <x86/page.h>

KERN_RET_CODE load_elf(ktask         *task , 
		       char          *fname ,
		       unsigned long *start_address,
		       unsigned long *u_stack
		       );



#endif // _LOADER_INTERNAL_H
