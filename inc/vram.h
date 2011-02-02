// Direct video RAM interfacing module

#ifndef __VRAM_H__
#define __VRAM_H__

#include "type.h"

#define VRAM_LINES            40
#define VRAM_COLS             80
#define VRAM_CHARS            ( VRAM_LINES * VRAM_COLS )
#define VRAM_SIZE_TOTAL       ( VRAM_CURSOR_SIZE + VRAM_LINES * VRAM_COLS * 2 )
#define VRAM_SIZE_VMEM_ONLY   ( VRAM_LINES * VRAM_COLS * 2 )
#define VRAM_LINE_SIZE        ( VRAM_COLS * 2 )
#define VRAM_CURSOR_SIZE      4

#define VRAM_TAB_SIZE         8

#define VRAM_CURSOR_OFF             0
#define VRAM_CURSOR_BLOCK           1
#define VRAM_CURSOR_BLOCK_BLINK     2
#define VRAM_CURSOR_LINE            5
#define VRAM_CURSOR_LINE_BLINK      6

// Colors
#define VRAM_COL_DONT_CHANGE        ( -1 )
#define VRAM_COL_DEFAULT            ( -2 )
enum
{
  VRAM_COL_BLACK,
  VRAM_COL_DARK_BLUE,
  VRAM_COL_DARK_GREEN,
  VRAM_COL_DARK_CYAN,
  VRAM_COL_DARK_RED,
  VRAM_COL_DARK_MAGENTA,
  VRAM_COL_BROWN,
  VRAM_COL_LIGHT_GRAY,
  VRAM_COL_DARK_GRAY,
  VRAM_COL_LIGHT_BLUE,
  VRAM_COL_LIGHT_GREEN,
  VRAM_COL_LIGHT_CYAN,
  VRAM_COL_LIGHT_RED,
  VRAM_COL_LIGHT_MAGENTA,
  VRAM_COL_LIGHT_YELLOW,
  VRAM_COL_WHITE
};

#define VRAM_DEFAULT_FG_COL         VRAM_COL_LIGHT_GRAY
#define VRAM_DEFAULT_BG_COL         VRAM_COL_BLACK

void vram_init();
void vram_send( int fd, char c );
void vram_putchar( char c );
void vram_set_color( int fg, int bg );
void vram_clrscr();
void vram_gotoxy( u8 x, u8 y );
void vram_deltaxy( int dx, int dy );
u8 vram_get_cx();
u8 vram_get_cy();
void vram_clreol();
void vram_set_cursor( int type );

#endif
