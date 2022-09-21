/** @file     faulthandlers.h
 *  @brief    This file exports the faulthandler initialization function
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */


#ifndef _FAULT_HANDLER
#define _FAULT_HANDLER
#include <kern_common.h>
#include <x86/page.h>
#include <task.h>

KERN_RET_CODE faulthandler_init();

#endif // _FAULT_HANDLER
