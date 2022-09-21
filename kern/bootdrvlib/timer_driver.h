/** @file timer_driver.h
 *  @brief Interfaces for the timer driver 
 *
 *  Header file containing interfaces for the timer driver.
 *
 *
 *  @author Faraz Shaikh (fshaikh)
 *  @bug No known bugs.
 */

#ifndef _TIMER_DRIVER_H
#define _TIMER_DRIVER_H

#include <kern_common.h>
#include <x86/pic.h>


/** @constant TIMER_DRIVER_XXX
 *  @brief constants for int acks and IDT placement
 */
#define TIMER_DRIVER_MASTER_ACK_IDX  0 
#define TIMER_DRIVER_IDT_IDX         X86_PIC_MASTER_IRQ_BASE
 
/** @type PTIMER_CALLBACK
 *  @brief type of the callback function called by the timer driver
 */
typedef void (*PTIMER_CALLBACK)(unsigned int);


//------------------------------------------------------------------------------
// Game exports see definitions for documentation
//------------------------------------------------------------------------------

KERN_RET_CODE 
timer_drv_init(void);

KERN_RET_CODE 
timer_set_callback(PTIMER_CALLBACK);

unsigned long 
timer_get_ticks();

#endif// _TIMER_DRIVER_H

