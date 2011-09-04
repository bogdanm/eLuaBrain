// Direct video RAM interfacing module

#ifndef __VRAM_H__
#define __VRAM_H__

#include "type.h"

#define VRAM_LINES            30
#define VRAM_COLS             80
#define VRAM_CHARS            ( VRAM_LINES * VRAM_COLS )
#define VRAM_SIZE_TOTAL       ( VRAM_CURSOR_SIZE + VRAM_LINES * VRAM_COLS * 2 )
#define VRAM_SIZE_VMEM_ONLY   ( VRAM_LINES * VRAM_COLS * 2 )
#define VRAM_LINE_SIZE        ( VRAM_COLS * 2 )
#define VRAM_CURSOR_SIZE      4

#define VRAM_TAB_SIZE         4

#define VRAM_CURSOR_OFF             0
#define VRAM_CURSOR_BLOCK           1
#define VRAM_CURSOR_BLOCK_BLINK     2
#define VRAM_CURSOR_LINE            5
#define VRAM_CURSOR_LINE_BLINK      6

// Box chars
#define VRAM_SBOX_UL                0xDA
#define VRAM_SBOX_UR                0xBF
#define VRAM_SBOX_BL                0xC0
#define VRAM_SBOX_BR                0xD9
#define VRAM_SBOX_HLINE             0xC4
#define VRAM_SBOX_VLINE             0xB3
#define VRAM_SBOX_CROSS             0xC5

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
int vram_get_cursor();
void* vram_box( unsigned x, unsigned y, unsigned width, unsigned height, const char *title, u16 flags );
void vram_close_box( void *pbox );
void vram_get_color( int *pfgcol, int *pbgcol );
void vram_enable_paging( int enabled );
void vram_set_last_line( int line );
void vram_change_attr( unsigned x, unsigned y, unsigned len, int newfg, int newbg );
void vram_set_mode( int mode );
int vram_get_fg( unsigned x, unsigned y );
int vram_get_bg( unsigned x, unsigned y );

#endif

