/** @file     bootdrivers.c
 *  @brief    This file defines the boot drivers initialization functions.
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 *  
 **/

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

#include "bootdrvlib/console_driver.h"
#include "bootdrvlib/timer_driver.h"    
#include "bootdrvlib/keyb_driver.h"    

/** @function boot_driver_init
 *  @brief    This function initializes the console driver,
 *            the timer driver and the keyboard driver
 *  @param    none
 *  @return   KERN_SUCCESS on success,
 *            boot driver error message if any initialization fails
 */

KERN_RET_CODE boot_driver_init(void)
{
  KERN_RET_CODE ret;
  FN_ENTRY();
  
  ret = console_drv_init();
  if( KERN_SUCCESS != ret ) {
    DUMP("Cannot Initialize console driver 0x%x",ret);
  }

  ret = timer_drv_init();
  if( KERN_SUCCESS != ret ) {
    DUMP("Cannot Initialize timer driver 0x%x",ret);
  }

  ret = keyb_drv_init();
  if( KERN_SUCCESS != ret ) {
    DUMP("Cannot Initialize keyboard driver 0x%x",ret);
  }
  
  FN_LEAVE();
  return ret; 
}
