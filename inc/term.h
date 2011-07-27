// Terminal functions

#ifndef __TERM_H__
#define __TERM_H__

#include "type.h"

// ****************************************************************************
// Data types

// Terminal output function
typedef void ( *p_term_out )( u8 );
// Terminal input function
typedef int ( *p_term_in )( int );
// Terminal translate input function
typedef int ( *p_term_translate )( int );

// Terminal input mode (parameter of p_term_in and term_getch())
#define TERM_INPUT_DONT_WAIT      0
#define TERM_INPUT_WAIT           1

// Maximum size on an ANSI sequence
#define TERM_MAX_ANSI_SIZE        14

// Colors
#define TERM_COL_DONT_CHANGE      ( -1 )
#define TERM_COL_DEFAULT          ( -2 )
enum
{
  TERM_COL_BLACK,
  TERM_COL_DARK_BLUE,
  TERM_COL_DARK_GREEN,
  TERM_COL_DARK_CYAN,
  TERM_COL_DARK_RED,
  TERM_COL_DARK_MAGENTA,
  TERM_COL_BROWN,
  TERM_COL_LIGHT_GRAY,
  TERM_COL_DARK_GRAY,
  TERM_COL_LIGHT_BLUE,
  TERM_COL_LIGHT_GREEN,
  TERM_COL_LIGHT_CYAN,
  TERM_COL_LIGHT_RED,
  TERM_COL_LIGHT_MAGENTA,
  TERM_COL_LIGHT_YELLOW,
  TERM_COL_WHITE
};

// This defines a box (window)
typedef struct
{
  u16 x, y, width, height;
  u16 flags;
  u8 *savedata;
} TERM_BOX;

// Foregound colors
#define TERM_PREFIX           "\x1b["
#define TERM_FGCOL_OFFSET     30
#define TERM_BGCOL_OFFSET     40
#define TERM_RESET_COL        TERM_PREFIX "0m"

// Foreground colors
#define TERM_FGCOL(x)               TERM_PREFIX x "m"
#define TERM_FGCOL_HIGH( col )      TERM_PREFIX "1m" TERM_PREFIX col "m"
#define TERM_FGCOL_BLACK            TERM_FGCOL( "30" )
#define TERM_FGCOL_DARK_RED         TERM_FGCOL( "31" )
#define TERM_FGCOL_DARK_GREEN       TERM_FGCOL( "32" )
#define TERM_FGCOL_DARK_YELLOW      TERM_FGCOL( "33" )
#define TERM_FGCOL_DARK_BLUE        TERM_FGCOL( "34" )
#define TERM_FGCOL_DARK_MAGENTA     TERM_FGCOL( "35" )
#define TERM_FGCOL_DARK_CYAN        TERM_FGCOL( "36" )
#define TERM_FGCOL_LIGHT_GRAY       TERM_FGCOL( "37" )
#define TERM_FGCOL_DARK_GRAY        TERM_FGCOL_HIGH( "30" )
#define TERM_FGCOL_LIGHT_RED        TERM_FGCOL_HIGH( "31" )
#define TERM_FGCOL_LIGHT_GREEN      TERM_FGCOL_HIGH( "32" )
#define TERM_FGCOL_LIGHT_YELLOW     TERM_FGCOL_HIGH( "33" )
#define TERM_FGCOL_LIGHT_BLUE       TERM_FGCOL_HIGH( "34" )
#define TERM_FGCOL_LIGHT_MAGENTA    TERM_FGCOL_HIGH( "35" )
#define TERM_FGCOL_LIGHT_CYAN       TERM_FGCOL_HIGH( "36" )
#define TERM_FGCOL_WHITE            TERM_FGCOL_HIGH( "37" )

#define term_cstr( x )              term_putstr( x, strlen( x ) )
#define term_cstrxy( x, y, s) do {\
  term_gotoxy( x, y );\
  term_putstr( s, strlen( s ) );\
  } while( 0 )

// Cursors
enum
{
  TERM_CURSOR_OFF,
  TERM_CURSOR_BLOCK,
  TERM_CURSOR_UNDERLINE
};

// Box flags
#define TERM_BOX_FLAG_BORDER        0x8000
#define TERM_BOX_FLAG_RESTORE       0x4000
#define TERM_BOX_FLAG_CENTER        0x2000

// Paging
#define TERM_PAGING_ON              1
#define TERM_PAGING_OFF             0

// ****************************************************************************
// Exported functions

// Terminal initialization
void term_init( unsigned lines, unsigned cols, p_term_out term_out_func, 
                p_term_in term_in_func, p_term_translate term_translate_func );

// Terminal output functions
void term_clrscr();
void term_clreol();
void term_gotoxy( unsigned x, unsigned y );
void term_up( unsigned delta );
void term_down( unsigned delta );
void term_left( unsigned delta );
void term_right( unsigned delta );
unsigned term_get_lines();
unsigned term_get_cols();
void term_putch( u8 ch );
void term_putstr( const char* str, unsigned size );
unsigned term_get_cx();
unsigned term_get_cy();
void term_set_color( int fgcol, int bgcol );
void term_get_color( int *pfgcol, int *pbgcol );
void term_set_cursor( int type );
int term_get_cursor();
void term_reset();
void* term_box( unsigned x, unsigned y, unsigned width, unsigned height, const char *title, u16 flags );
void term_close_box( void *pbox );
void term_enable_paging( int enabled );
void term_set_last_line( int line );
void term_change_attr( unsigned x, unsigned y, unsigned len, int newfg, int newbg );

#define TERM_KEYCODES\
  _D( KC_UP ),\
  _D( KC_DOWN ),\
  _D( KC_LEFT ),\
  _D( KC_RIGHT ),\
  _D( KC_HOME ),\
  _D( KC_END ),\
  _D( KC_PAGEUP ),\
  _D( KC_PAGEDOWN ),\
  _D( KC_ENTER ),\
  _D( KC_TAB ),\
  _D( KC_BACKSPACE ),\
  _D( KC_ESC ),\
  _D( KC_CTRL_Z ),\
  _D( KC_CTRL_A ),\
  _D( KC_CTRL_E ),\
  _D( KC_CTRL_C ),\
  _D( KC_CTRL_T ),\
  _D( KC_CTRL_U ),\
  _D( KC_CTRL_K ),\
  _D( KC_CTRL_Y ),\
  _D( KC_CTRL_B ),\
  _D( KC_CTRL_X ),\
  _D( KC_CTRL_V ),\
  _D( KC_DEL ),\
  _D( KC_F1 ),\
  _D( KC_F2 ),\
  _D( KC_F3 ),\
  _D( KC_F4 ),\
  _D( KC_F5 ),\
  _D( KC_F6 ),\
  _D( KC_F7 ),\
  _D( KC_F8 ),\
  _D( KC_F9 ),\
  _D( KC_F10 ),\
  _D( KC_F11 ),\
  _D( KC_F12 ),\
  _D( KC_CTRL_F2 ),\
  _D( KC_CTRL_F4 ),\
  _D( KC_INSERT ),\
  _D( KC_UNKNOWN )
  
// Terminal input functions
// Keyboard codes
#define _D( x ) x

enum
{
  term_dummy = 255,
  TERM_KEYCODES,
  TERM_FIRST_KEY = KC_UP,
  TERM_LAST_KEY = KC_UNKNOWN
};

int term_getch( int mode );

#endif // #ifndef __TERM_H__
