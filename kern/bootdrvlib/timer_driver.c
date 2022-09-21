/** @file     timer_driver.c 
 *  @brief    This file contains the implementation of a timer driver
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <video_defines.h>
#include <simics.h>
#include <asm.h>

#include <kern_common.h>
#include "i386lib/i386systemregs.h"
#include <timer_defines.h>

#include "timer_driver.h"
#include "keyb_driver.h"

/** @type  TERM_DRIVER_STATE
 *  @brief the state of a running timer driver
 */
typedef struct _TERM_DRIVER_STATE {
  unsigned long        ticks;       //- a.k.a jiffies -//
  PTIMER_CALLBACK      callback;    //- must be array to chain callback-//
}TIMER_DRIVER_STATE; 


/** @global timer_driver_state
 *  @brief  global instance of timer driver state
 */
TIMER_DRIVER_STATE timer_driver_state; 




/** @function  _BASE_TIMER_CALL_BACK
 *  @brief     This is the first C function called from our hot patched idt entry
 *             Acks the timer interupt and updates jiffies state
 *  @note      should be called with timer interrupt disabled
 *  @param     none
 *  @return    void
 */

void static _BASE_TIMER_CALL_BACK(void) {
  FN_ENTRY();

  DEBUG_PRINT("Timer driver called");
  timer_driver_state.ticks++;
  if(0 == timer_driver_state.ticks)
    DUMP("Overflows: Too many ticks");

  //-- execute driver bottom halves --//
  //keyb_bottom_half();
  sleep_bottom_half();
  
  pic_acknowledge(TIMER_DRIVER_MASTER_ACK_IDX);
  
  if( NULL != timer_driver_state.callback )
    timer_driver_state.callback(timer_driver_state.ticks);


  FN_LEAVE();
}



/** @function  timer_set_callback
 *  @brief     This function sets higher level timer callback to be called from base ISR
 *  @note      currently over-writes the existing handler
 *  @param     pointer to timer callback function
 *  @return    KERN_SUCCESS
 */

KERN_RET_CODE timer_set_callback(PTIMER_CALLBACK ptimer_callback) {
  FN_ENTRY();
 
  timer_driver_state.callback = ptimer_callback;

  FN_LEAVE();
  return KERN_SUCCESS;
}



/** @function  timer_get_ticks
 *  @brief     This function returns number of timer interupts since boot
 *  @param     none
 *  return     the number of ticks since boot
 */

unsigned long timer_get_ticks() {
  return timer_driver_state.ticks;
}



/** @function  timer_drv_init
 *  @brief     This function initializes timer driver internal state
 *             This function also installs the timer driver ISR in IDT
 *  @param     none
 *  @return    KERN_SUCCESS if success
 *             KERN error code if failure
 */

#define SCALING_FACTOR 10
#define SHIFT_8        8
#define MASK_LSB 0xff
KERN_RET_CODE timer_drv_init(void) {
  KERN_RET_CODE ret;
  //unsigned int timer_out_lsb,timer_out_msb;
  //int new_rate  = TIMER_RATE / SCALING_FACTOR;
  FN_ENTRY();

  //-- Reset the internal state of the timer --//
  memset( &timer_driver_state , 0 , sizeof(timer_driver_state) );

  //  timer_out_lsb = new_rate & MASK_LSB;
  //timer_out_msb = (new_rate >> SHIFT_8) & MASK_LSB;

  //outb(TIMER_MODE_IO_PORT ,TIMER_SQUARE_WAVE);
  //outb(TIMER_PERIOD_IO_PORT,timer_out_lsb);
  //outb(TIMER_PERIOD_IO_PORT,timer_out_msb);


  //-- Install the handler --//
  ret = i386_install_isr(_BASE_TIMER_CALL_BACK,
			 TIMER_DRIVER_IDT_IDX,
			 i386_GATE_TYPE_INTR,
			 i386_PL0);






  FN_LEAVE();
  return ret;
}

