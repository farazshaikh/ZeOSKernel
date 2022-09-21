/** @file     keyb_driver.c 
 *  @brief    This file contains the implementation of a keyboard driver
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <types.h>

#include <video_defines.h>
#include <simics.h>
#include <asm.h>

#include <kern_common.h>
#include <interrupt_defines.h>
#include "i386lib/i386systemregs.h"
#include "keyb_driver.h"
#include "bootdrvlib/console_driver.h"

#define  CAT_KEYB_KEYS_NUM 110
#define  READ_LINE_CHARACTER 10

/** @typedef  KEYB_DRIVER_STATE
 *  @brief    describes the sate of a running keyboard driver
 */

typedef struct _KEYB_DRIVER_STATE {
  semaphore           wait_for_chars; 
  semaphore           wait_for_readline; 
  
  unsigned int        keys_pressed_nr;  //- 4 entropy pool -//

  //- spinlock for protecting this stuff -//
  spinlock            keyboard_state_lock;
  

  //-- Raw scan codes --//
  char                keyb_ring_buffer[CAT_KEYB_KEYS_NUM]; 
  volatile int        head;
  volatile int        tail;
  
  //-- processed scan codes --//
  char                keys_ring_buffer[CAT_KEYB_KEYS_NUM];
  volatile int        keys_head;
  volatile int        keys_tail;
}KEYB_DRIVER_STATE; 


/** @global  keyb_driver_state
 *  @brief   global state of our keyboard driver
 */

KEYB_DRIVER_STATE keyb_driver_state; 

uint32_t  KEYBOARD_STATE_LOCK()  {				
    uint32_t _local_eflags;					
    _local_eflags  = spinlock_ifsave(&keyb_driver_state.keyboard_state_lock); 
    _local_eflags  = _local_eflags;					
    return _local_eflags;
}

void  KEYBOARD_STATE_UNLOCK(uint32_t _eflags) {
  spinlock_ifrestore(&keyb_driver_state.keyboard_state_lock,(_eflags)); 
}

/** @function  keyb_raw_buffer_enqueue
 *  @brief     This function enqueues a raw scancode into the keyboard ring buffer
 *  @NOTE      drops scan code if ring buffer is full
 *  @param     scan_code - the raw scan code as obtained from the keystroke
 *  @return    void
 */

static void keyb_raw_buffer_enqueue(char scan_code) { 
  FN_ENTRY();

  if( (keyb_driver_state.head+1) % CAT_KEYB_KEYS_NUM == keyb_driver_state.tail ) {
    FN_LEAVE();
    DUMP("Keyboard ring buffer full,dropping scancode %c",scan_code);
    return;
  }
  
  //-- Move ahead the head --//
  keyb_driver_state.keyb_ring_buffer[keyb_driver_state.head] = scan_code; 
  keyb_driver_state.head++;
  keyb_driver_state.head %= CAT_KEYB_KEYS_NUM; 

  
  FN_ENTRY();
}


/** @function  keyb_raw_buffer_dequeue
 *  @brief     This function dequeues a raw scancode from the keyboard ring buffer
 *  @param     none
 *  @return    the scan code value if available           
 *             -1 if the ring buffer empty
 */

static int keyb_raw_buffer_dequeue(void) { 
  int curr_char; 
  FN_ENTRY();
  
  if( keyb_driver_state.head == keyb_driver_state.tail ) {
    DEBUG_PRINT("Keyboard buffer empty");
    FN_LEAVE();
    return -1;
  }

  //- Get the current character --//            //-- Atomic INC on i386 --//
  curr_char = keyb_driver_state.keyb_ring_buffer[keyb_driver_state.tail++];
  keyb_driver_state.tail %= CAT_KEYB_KEYS_NUM;
  
  FN_LEAVE();
  return( curr_char );
}


/** @function  keyb_processed_buffer_enqueue
 *  @brief     This function enqueues a character into the keyboard ring buffer
 *  @NOTE      drops scan code if ring buffer is full
 *  @param     key - processed ASCII character value (printable characters only)
 *  @return    void
 */

static void keyb_processed_buffer_enqueue(char key) { 
  FN_ENTRY();

  if( (keyb_driver_state.keys_head+1) % CAT_KEYB_KEYS_NUM == keyb_driver_state.keys_tail ) {
    FN_LEAVE();
    DUMP("Keyboard processed ring buffer full,dropping character %c",key);
    return;
  }

  //-- Move ahead the head --//
  if( key == '\b' ) {
    if( keyb_driver_state.keys_head == keyb_driver_state.keys_tail )
      return;
    keyb_driver_state.keys_head--;
    keyb_driver_state.keys_head %= CAT_KEYB_KEYS_NUM; 
    keyb_driver_state.keys_ring_buffer[keyb_driver_state.keys_head] = '\0'; 
  }
  else {
    keyb_driver_state.keys_ring_buffer[keyb_driver_state.keys_head] = key; 
    keyb_driver_state.keys_head++;
    keyb_driver_state.keys_head %= CAT_KEYB_KEYS_NUM; 
  }
  FN_LEAVE();
}



/** @function  keyb_processed_buffer_dequeue
 *  @brief     This function dequeues a scancode from the keyboard ring buffer
 *  @param     none
 *  @return    processed key value if available           
 *             -1 if ring buffer empty
 */

static int keyb_processed_buffer_dequeue(void) { 
  int curr_char; 
  FN_ENTRY();
  
  if( keyb_driver_state.keys_head == keyb_driver_state.keys_tail ) {
    DEBUG_PRINT("Keyboard processed buffer empty");
    FN_LEAVE();
    return -1;
  }

  //- Get the current character --//            //-- Atomic INC on i386 --//
  curr_char = keyb_driver_state.keys_ring_buffer[keyb_driver_state.keys_tail++];
  keyb_driver_state.keys_tail %= CAT_KEYB_KEYS_NUM;
  
  //-- Move ahead the head --//
  return( curr_char );
  FN_LEAVE();
}




/** @function  keyb_acknowledge_interupt
 *  @brief     This function acknowledges the keyboard driver 
 *             the about delivery of interupt, 
 *             clears in service request bits ISR bits in 8259
 *  @param     none
 *  @return    void           
 */

static inline void keyb_acknowledge_interupt(void) {
  outb( INT_CTL_PORT , INT_ACK_CURRENT );
}



/** @function  _BASE_KEYB_CALL_BACK
 *  @brief     This function first C function called from our hot patched idt entry
 *             Acks the keyb interupt and retrieves and enqueues scancode
 *  @param     none
 *  @return    void
 */

void static _BASE_KEYB_CALL_BACK(void) {
  char       scan_code; 
  uint32_t   eflags;
  FN_ENTRY();
  scan_code = inb(KEYBOARD_PORT);

  eflags = KEYBOARD_STATE_LOCK();
  keyb_raw_buffer_enqueue(scan_code);

  //-- Add to the number of recieved key strokes --//
  keyb_driver_state.keys_pressed_nr++;
  KEYBOARD_STATE_UNLOCK(eflags);
  

  keyb_acknowledge_interupt();

  keyb_bottom_half();
  
  FN_LEAVE();
}



/** @function  keystroke_cnt_entropy
 *  @brief     This function counts the number of keyboard interrupts since boot
 *             used as entropy pool for random number seed generation
 *  @param     none
 *  @return    the number of keys pressed maintained in the driver state
 */

unsigned long keystroke_cnt_entropy(void) {
  return keyb_driver_state.keys_pressed_nr;
}



/** @function  keyb_drv_init
 *  @brief     This function initializes the keyboard driver
 *             This function also installs the ISR as the IDT entry
 *  @param     none
 *  @return    KERN_SUCCESS if installation is successful,
 *             else returns the installation failure error code
 */

KERN_RET_CODE keyb_drv_init(void) {
  KERN_RET_CODE ret;
  FN_ENTRY();

  //-- Reset the internal state of the timer --//
  memset( &keyb_driver_state , 0 , sizeof(keyb_driver_state) );
  SEMAPHORE_INIT(&keyb_driver_state.wait_for_chars,0);
  SEMAPHORE_INIT(&keyb_driver_state.wait_for_readline,0);
  SPINLOCK_INIT(&keyb_driver_state.keyboard_state_lock);
  //-- Install the handler --//
  ret = i386_install_isr(_BASE_KEYB_CALL_BACK,
			 KEYB_DRIVER_IDT_IDX,
			 i386_GATE_TYPE_INTR,
			 i386_PL0);

  FN_LEAVE();
  return ret;
}



/** @function  readchar
 *  @brief     This function dequeues the scan code from the raw buffer
 *             then processes the scan code into a readable/printable character
 *  @param     none
 *  @return    key character - if keyb ring buffer has a valid char
 *              -1 if the keyb ring buffer is empty
 *  @note      this is only function that calls process_scancode
 *             process_scancode has internal state and must be called with
 *             inorder scancodes
 */

int
readchar(void)
{
  int scan_code;
  int aug_char; 
  uint32_t eflags;

  eflags = KEYBOARD_STATE_LOCK();
  scan_code = keyb_raw_buffer_dequeue();
  KEYBOARD_STATE_UNLOCK(eflags);
  if( scan_code == -1 ) {
    // BIG FAT NOTE: You have to return from here 
    // please don't pass the scan_code -1 to to process_scancode as its internal
    // state machine for detecting extended codes goes out of sync
    return scan_code;    
  }


  assert( -1 != scan_code );

  //- get the augmented process code --//
  aug_char = process_scancode( scan_code );

  //- If its a character return the character -//
  if( KH_HASDATA(aug_char) && KH_ISMAKE(aug_char) ) {
    return KH_GETCHAR( aug_char ); //-- char value to be send only on make --//
  }else {
    //-- If return RAW code --//
    return -1;                    //-- Other lets put out the aug_char as is --//
  }
}

/** @function  synchronous_readchar
 *  @brief     This function dequeues a character from the processed char buffer
 *  @param     none
 *  @return    key character - wait if the buffer is empty but return a char
 */

int synchronous_readchar() {
  int keyb_char;
  sem_wait(&keyb_driver_state.wait_for_chars);
  keyb_char = keyb_processed_buffer_dequeue();
  DEBUG_PRINT("Keyboard characters is %d ",keyb_char);
  return keyb_char;
}

/** @function  synchronous_readline
 *  @brief     This function dequeues multiple characters from the processed buffer
 *             then enqueues them into the supplied buffer 
 *             until the supplied buffer is filled or keyboard buffer becomes empty
 *  @param     len  - the length of the supplied buffer (number of characters)
 *  @param     buff - the pointer to the start of buffer
 *  @return    the number of characters pushed into the buffer 0=<i<=len 
 */

int synchronous_readline(int len,char *buff) {
  int keyb_char;
  int i=0;
  sem_wait(&keyb_driver_state.wait_for_readline);

  while(i<len) {
    keyb_char = keyb_processed_buffer_dequeue();

    DEBUG_PRINT("Keyboard characters is %d ",keyb_char);
    buff[i++] = (char)keyb_char;

    if(keyb_char == READ_LINE_CHARACTER) 
      break;
  }

  // drain if the user buffer was smaller than readline
  if(keyb_char != READ_LINE_CHARACTER) {
    do {
      keyb_char = keyb_processed_buffer_dequeue();
    }while(keyb_char != READ_LINE_CHARACTER && keyb_char != -1);
  }
  
  return i;
}

/** @function  keyb_bottom_half
 *  @brief     This function handles portion of the keyboard driver state 
 *             that is not handled by the keyboard ISR.
 *             This function reads a processed scancode (character)
 *             if printable/readable, it prints the character on the terminal
 *             then enqueues the character into the processed buffer
 *             to be used by synchronous readers
 *  @param     none
 *  @return    key character - printable/unprintable(-1) character value
 */

int keyb_bottom_half() { 
  int readycharacter; 
  readycharacter = readchar(); 
  if(readycharacter == -1) 
    return readycharacter;

  //-- We have a ready characters --//
  if( readycharacter == '\b' ) {
    if( keyb_driver_state.keys_head == keyb_driver_state.keys_tail )
      return readycharacter;
    else
      handle_backspace();
  }
  else
    putbytes((char *)&readycharacter,1);

  //-- Wake up waiters if any     --//
  keyb_processed_buffer_enqueue((char)readycharacter);
  if( (sem_waiters(&keyb_driver_state.wait_for_readline))
      &&(readycharacter == READ_LINE_CHARACTER)) {
    sem_signal(&keyb_driver_state.wait_for_readline);
    return 0;
  }

  if(!sem_waiters(&keyb_driver_state.wait_for_readline)) {
    sem_signal(&keyb_driver_state.wait_for_chars);
  }
  
  return readycharacter;
}
