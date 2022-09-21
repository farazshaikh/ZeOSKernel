/** @file     console_driver.c 
 *  @brief    This file contains the implementation of a 80x25 tty console driver
 *
 *  @author   Faraz Shaikh (fshaikh)
 *  @bug      No known bugs
 */

#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include <simics.h>
#include <asm.h>

#include <kern_common.h>
#include "bootdrvlib/console_driver.h"
#include "bootdrvlib/timer_driver.h"    //-- for Compatibility with test kernel --//
#include "bootdrvlib/keyb_driver.h"     //-- for Compatibility with test kernel --//


/*******************************************************************************
 * Internal Helpers. 
 *******************************************************************************
 */

/** @global  term_driver_state
 *  @brief   our terminal driver state variable definition
 */

TERM_DRIVER_STATE term_driver_state; 

#define PROMPT_LIMIT 13

/** @macros  term_get_XXXX
 *  @brief   convinient wrappers for accessing terminal driver's state
 */

#define term_get_rpos  (term_driver_state.rpos)
#define term_get_cpos  (term_driver_state.cpos)
#define term_get_frame (term_driver_state.framebuffer)
#define term_get_color (term_driver_state.termcolor)
#define term_cursor_visible (term_driver_state.cursor_visible)


/** @typedef  linear_cursor_pos
 *  @brief    Convinent wrapper for breaking up the linear console position
 */

union linear_cursor_pos {
  unsigned short cursorpos;   //- linear postion of the cursor -//
  struct {
    char     lowbyte;         //- lower byte of the cursor position -//
    char     highbyte;        //- higher byte of the cursor position -//
  }u;
}PACKED;




/** @function draw_crtc_cursor
 *  @brief    displays the cursor according to driver state
 *  @param    row - row idx to display the cursor
 *  @param    col - column idx to display the cursor
 *  @return   void
 */

static void draw_crtc_cursor( 
			     int row ,
			     int column
			     )
{
  union linear_cursor_pos cpos;
  FN_ENTRY();

  //- Is the cursor visible -//
  if(FALSE == term_cursor_visible) {
    FN_LEAVE();
    return; 
  }
  
  //-- Compute the row and column --//
  //-- Sanity checks are already done in the calling functions -//
  cpos.cursorpos = (row * CONSOLE_WIDTH) + column;
  DEBUG_PRINT("Cursor Pos %u %u %u",cpos.cursorpos,cpos.u.lowbyte,cpos.u.highbyte);

  //- Pass the state out to the CRTC ports -//
  outb(CRTC_IDX_REG,CRTC_CURSOR_LSB_IDX);
  outb(CRTC_DATA_REG,cpos.u.lowbyte);
  outb(CRTC_IDX_REG,CRTC_CURSOR_MSB_IDX);
  outb(CRTC_DATA_REG,cpos.u.highbyte);

  FN_LEAVE();
}


/*******************************************************************************
 *Exported functions. 
 *******************************************************************************
 */

/** @function console_drv_get_frame
 *  @brief    returns a pointer to the console frame buffer
 *  @param    none
 *  @return   pointer to the frame buffer
 */

PFRAME_BUFFER console_drv_get_frame() {

  //-- wrong call sequence --//
  if(NULL == term_driver_state.framebuffer)
    assert(0);   
    
  return term_driver_state.framebuffer;
}



/** @function console_driver_init
 *  @brief    initializes the console driver
 *  @param    none
 *  @returns  KERN_SUCCESS on conole driver initialization success
 */

KERN_RET_CODE console_drv_init(void) {
  FN_ENTRY();

  //- Internal State -//
  memset(&term_driver_state,0,sizeof(term_driver_state));
  
  //- Make framebuffer ptr ready to be accessed in normal C style -//
  term_driver_state.framebuffer = (PFRAME_BUFFER) CONSOLE_MEM_BASE; 
  clear_console();
  set_term_color(FGND_GREEN | BGND_BLACK);
  show_cursor();
  putbytes(OS_NAME,strlen(OS_NAME));

  FN_LEAVE();
  return KERN_SUCCESS;
}



/** @function handler_install
 *  @brief    initialized the console driver
 *  @NOTE     this function is only used by the test kernel
 */

int handler_install(void (*tickback)(unsigned int))
{
  KERN_RET_CODE ret;
  FN_ENTRY();

  //-- Initialize drivers --//
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

  //-- Install the test kernel's timer ISR --//
  ret = timer_set_callback(tickback);
  if( KERN_SUCCESS != ret ) {
    DUMP("Cannot set timer callback 0x%x",ret);
  }


  
  FN_LEAVE();

  return ret; 
}


/** @function page_scroll
 *  @brief    This function emulates scrolling of the screen
 *            This is achieved by moving all characters in the screen buffer
 *            up by one row at a time, thus losing the top row
 *            to make way for the new row at the bottom.
 *  @param    none
 *  @return   void
 */

void page_scroll(){
  int i;
  char *addr = (char *) CONSOLE_MEM_BASE;

  /*scroll last n-1 rows up by one row, taking one column at a time*/
  for( i = 0 ; i < CONSOLE_HEIGHT - 1 ; i++ )
    memcpy( (addr + i*2*CONSOLE_WIDTH) , (addr + (i+1)*2*CONSOLE_WIDTH) , 2*CONSOLE_WIDTH );

  /*blank the last (new) row*/
  memset( (addr + (CONSOLE_HEIGHT - 1)*2*CONSOLE_WIDTH) , 0 , 2*CONSOLE_WIDTH );
 
}



/** @function handle_backspace
 *  @brief    This function emulates backspace character handling
 *            This is achieved by moving one character back in the screen buffer
 *            and writing a SPACE over the existing character
 *  @param    none
 *  @return   void
 */

void handle_backspace(){
  /*scroll last n-1 rows up by one row, taking one column at a time*/
    term_get_cpos--;
    if( term_get_cpos < 0 ) {
      term_get_rpos--;
      term_get_cpos = CONSOLE_WIDTH - 1;
    }
    term_get_frame[term_get_rpos][term_get_cpos].val = SPACE_VALUE;
    term_get_frame[term_get_rpos][term_get_cpos].attr = term_get_color;
}



/** @function putbyte
 *  @brief    places the single character on the frame buffer
 *            (displays the character on the terminal)
 *  @param    ch - value of the character to be places
 *  @return   KERN_SUCCESS on successfully printing the char ch
 */

int putbyte( char ch )
{
  FN_ENTRY();
  
  //- Debugging -//
  DEBUG_PRINT("CHAR AT ADDRESS %p %c",
	       &term_get_frame[term_get_rpos][term_get_cpos].val,ch);

  //- Handle Special Characters first -//
  switch( ch ) {

  case '\n':
    if( (CONSOLE_HEIGHT - 1) == term_get_rpos )
      page_scroll();
    else
      term_get_rpos++;
    
    //-- BIG_FAT_NOTE: There is no break here --//
    //-- EMULATES \n\r for \n -//

  case '\r':
    term_get_cpos = 0;
    break;
    
  case '\b':
    handle_backspace();
    break;

  default:
    //- default printing + cursor positioning --//
    term_get_frame[ term_get_rpos ][ term_get_cpos ].val = ch;
    term_get_frame[ term_get_rpos ][ term_get_cpos ].attr = term_get_color;
    term_get_cpos++; 
    if( term_get_cpos == CONSOLE_WIDTH ) {
      term_get_cpos = 0;
      if(term_get_rpos == (CONSOLE_HEIGHT - 1) )
	page_scroll();
      else
	term_get_rpos++;

    }  
  }

  //-- Draw the cursor --//
  draw_crtc_cursor( term_get_rpos , term_get_cpos );

  //- Sanity checks for fadango on frame      -//
  assert( term_get_cpos >=0 && term_get_cpos < CONSOLE_WIDTH );
  assert( term_get_rpos >=0 && term_get_rpos < CONSOLE_HEIGHT );
  
  //-- return                                --//
  FN_LEAVE();
  return KERN_SUCCESS; 
}



/** @function putbytes
 *  @brief    places the stream of characters on the frame buffer
 *  @param    s   - string of bytes to be transferred onto the buffer
 *  @param    len - count of characters to output from s
 *  @return   void
 */

void 
putbytes( 
	 const char *s,
	 int len 
	 )
{
  int i;
  FN_ENTRY();

  //- Print individual characters -//
  for( i = 0 ; i < len ; i++ ) {
    putbyte( s[i] );
  }

  FN_LEAVE();
}



/** @function set_term_color
 *  @brief    This function sets the global screen rendering attribute
 *  @param    color - value describing the attribute
 *  @return   void
 */

int
set_term_color( int color )
{
  FN_ENTRY();
  term_get_color = color;
  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function get_term_color
 *  @brief    This function gets global screen rendering attribute
 *  @param    color - pointer to int location that get the attribute value
 *  @return   void
 */

void
get_term_color( int *color )
{
  FN_ENTRY();

  *color = term_get_color;

  FN_LEAVE();
}



/** @function set_cursor
 *  @brief    This function sets the cursor position
 *  @param    row - row index to place the cursor at
 *  @param    col - column index to place the cursor at
 *  @return   void
 */

int
set_cursor(
	   int row,
	   int col
	   )
{
  FN_ENTRY();

 //- param check -//
  if( row < 0 || row >= CONSOLE_HEIGHT ) {
    return KERN_ERROR_CURSOR_FADANGO;
  }
  if( col < 0 || col >= CONSOLE_WIDTH ) {
    return KERN_ERROR_CURSOR_FADANGO;
  }

  term_get_rpos = row;
  term_get_cpos = col;
  draw_crtc_cursor(term_get_rpos,term_get_cpos);

  FN_LEAVE();
  return KERN_SUCCESS;
}


/** @function  get_cursor
 *  @brief     This function gets the current position of the cursor
 *  @param     row - pointer to integer holding the row index
 *  @param     col - pointer to integer holding the column index
 *  @return    void
 */

void
get_cursor(
	   int *row ,
	   int *col
	   )
{
  FN_ENTRY();

  *row = term_get_rpos;
  *col = term_get_cpos;

  FN_LEAVE();
}


/** @function  hide_cursor
 *  @brief     This function directs the CRTC to hide the cursor
 *  @param     none
 *  @return    void
 */

void
hide_cursor()
{
  FN_ENTRY();

  draw_crtc_cursor( CONSOLE_HEIGHT ,CONSOLE_WIDTH + 1);
  term_driver_state.cursor_visible = FALSE;

  FN_LEAVE();
}



/** @function  show_cursor
 *  @brief     This function directs the CRTC to make the cursor visible
 *  @param     none
 *  @return    void
 */

void
show_cursor()
{
  FN_ENTRY();

  term_driver_state.cursor_visible = TRUE;
  draw_crtc_cursor( term_get_rpos , term_get_cpos );

  FN_LEAVE();
}


/** @function  clear_console
 *  @brief     This function clears the frame buffer 
 *             console using SPACE_VALUEs (' ')
 *  @param     none
 *  @return    void
 */

void 
clear_console()
{
  unsigned int i,j;
  FN_ENTRY();
  for(i = 0 ; i < CONSOLE_HEIGHT ; i++) {
    for(j = 0 ; j < CONSOLE_WIDTH ; j++) {
      term_get_frame[i][j].val = SPACE_VALUE;
      term_get_frame[i][j].attr = term_get_color;
    }
  }

  term_get_cpos = term_get_rpos = 0;
  draw_crtc_cursor(term_get_rpos,term_get_rpos);
  FN_LEAVE();
}



/** @function  draw_char
 *  @brief     places character with specified attributes and location in frame
 *  @param     row   - row index of the character 
 *  @param     col   - column index of the character
 *  @param     ch    - the character to place
 *  @param     color - attribute of the character to be placed
 *  @return    void
 */

void
draw_char( 
	  int row, 
	  int col, 
	  int ch, 
	  int color
	  )
{
  FN_ENTRY();

  //- param check -//
  if(row < 0 || row >= CONSOLE_HEIGHT) {
    assert(0);
  }
  if(col < 0 || col >= CONSOLE_WIDTH) {
    assert(0);
  }
  
  //- set the character value and the colour -//
  term_get_frame[row][col].val = ch;
  term_get_frame[row][col].attr = color;

  FN_LEAVE();
}



/** @function  get_char
 *  @brief     fetches current character value from the frame buffer
 *  @param     row - row index of the character 
 *  @param     col - column index of the character
 *  @return    the value of the character at (row,col)
 */

char
get_char( 
	 int row,
	 int col
	 )
{
  FN_ENTRY();

  //- param check -//
  if(row < 0 || row >= CONSOLE_HEIGHT) {
    return KERN_ERROR_CURSOR_FADANGO;
  }
  if(col < 0 || col >= CONSOLE_WIDTH) {
    return KERN_ERROR_CURSOR_FADANGO;
  }
  
  //- return the value at the position -//
  FN_LEAVE();
  return term_get_frame[row][col].val;
}

/** @function  get_cmd_line
 *  @brief     fetches the last executed command in the shell
 *  @param     buf - return buffer
 *  @param     len - length of the buffer
 *  @return    KERN_SUCCESS on success, KERN_ERROR_CURSOR_FADANGO on fail
 */

int
get_cmd_line(
	 char *buf,
	 int len
	 )
{
  int row, col, i = 0;
  char ch;
  get_cursor( &row , &col );
  row -= 1;
  col = 13; // -- strlen(shell_prompt); -- //
  FN_ENTRY();

  do {
    if(( ch = get_char( row , col++ )) == KERN_ERROR_CURSOR_FADANGO )
      return KERN_ERROR_CURSOR_FADANGO;
    if( ch != ' ' )
      buf[i++] = ch;
    else
      buf[i++] = '\0';
    if( i == len - 4 ) {
      buf[i++] = '.'; buf[i++] = '.'; buf[i++] = '.';
      break;
    }
  }while( ch != ' ' ); 
  
  //- return the value at the position -//
  FN_LEAVE();
  return KERN_SUCCESS;
}

