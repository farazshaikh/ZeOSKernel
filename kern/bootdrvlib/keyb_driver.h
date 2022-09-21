/** @file keyb_driver.h
 *  @brief Interfaces for the keyboard driver 
 *
 *  Header file containing interfaces for the keyboard driver.
 *
 *
 *  @author Faraz Shaikh (fshaikh)
 *  @bug No known bugs.
 */

#ifndef _KEYB_DRIVER_H
#define _KEYB_DRIVER_H

#include <kern_common.h>
#include <x86/pic.h>
#include <keyhelp.h>

/** @constant KEYB_DRIVER_XXX
 *  @brief constants for int acks and IDT placement
 */
#define KEYB_DRIVER_IDT_IDX         (KEY_IDT_ENTRY)
#define KEYB_DRIVER_MASTER_ACK_IDX  (KEY_ID_ENTRY - ADDR_PIC_BASE) 

//------------------------------------------------------------------------------
// Game exports see definitions for documentation
//------------------------------------------------------------------------------

KERN_RET_CODE 
keyb_drv_init(void);

unsigned long 
keystroke_cnt_entropy(void);

int           
readchar(void);

int keyb_bottom_half();
int synchronous_readchar();
int synchronous_readline(int len,char *buff);
#endif// _KEYB_DRIVER_H
