/** @file     console.h
 *  @brief    This file exports the console driver function
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <video_defines.h>

// -- For the documentation of following functions -- //
// -- refer to kern/bootdrvlib/console_driver.c    -- //
// -- where the below functions are defined        -- //

int putbyte( char ch );

void putbytes(const char* s, int len);

int set_term_color(int color);

void get_term_color(int* color);

int set_cursor(int row, int col);

void get_cursor(int* row, int* col);

void hide_cursor();

void show_cursor();

void clear_console();

void page_scroll();

void handle_backspace();

void draw_char(int row, int col, int ch, int color);

char get_char(int row, int col);

int get_cmd_line(char *buf, int len);

// -- end of exported function list                -- //

#endif /* _CONSOLE_H */
