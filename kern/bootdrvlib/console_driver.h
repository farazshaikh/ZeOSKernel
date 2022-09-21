/** @file console_driver.h
 *  @brief Interfaces for the console driver 
 *
 *  Header file containing interfaces for the console driver.
 *
 *
 *  @author Faraz Shaikh (fshaikh)
 *  @bug No known bugs.
 */

#ifndef _CONSOLE_DRIVER_H
#define _CONSOLE_DRIVER_H

#include <video_defines.h>


/** @constant SPACE_VALUE
 *  @brief    used for clear screen
 */
#define SPACE_VALUE 0x20


/** @type  TERM_CHAR
 *  @brief Access a slot in the frame buffer
 */
struct _TERM_CHAR  {
  char val;   //- Value of the character to be printed -//
  char attr;//- Color specification of the character to be printed -//
}PACKED;
typedef struct _TERM_CHAR TERM_CHAR; 



/** @type  PFRAME_BUFFER
 *  @brief pointer type for array style access to the frame buffer.
 */

typedef TERM_CHAR (*PFRAME_BUFFER)[CONSOLE_WIDTH];



/** @type  TERM_DRIVER_STATE
 *  @brief current state of the terminal driver
 */

typedef struct _TERM_DRIVER_STATE {
  int rpos; //- X position of the cursor 0 - (CONSOLE_WIDTH -1  )-//
  int cpos; //- Y position of the cursor 0 - (CONSOLE_HEIGHT - 1)-//
  int termcolor; //- Colour of the terminal -//
  int cursor_visible;
  PFRAME_BUFFER framebuffer; //- pointer to the frame buffer -// 
}TERM_DRIVER_STATE; 



//------------------------------------------------------------------------------
// Game exports see definitions for documentation
//------------------------------------------------------------------------------

PFRAME_BUFFER
console_drv_get_frame(); 

int 
handler_install(void (*tickback)(unsigned int));

KERN_RET_CODE
console_drv_init(void);

int 
putbyte( char ch );

void 
putbytes( const char *s, int len );

int
set_term_color( int color );

void
get_term_color( int *color );

int
set_cursor( int row, int col );

void
get_cursor( int *row, int *col );

void
hide_cursor();

void
show_cursor();

void 
clear_console();

void
draw_char( int row, int col, int ch, int color );

char
get_char( int row, int col );

#endif //_CONSOLE_DRIVER_H
