// Editor hardware related functions

#include "edhw.h"
#include "editor.h"
#include "type.h"
#include "term.h"
#include "platform_conf.h"
#include <string.h>

#define TEXTCOL       TERM_COL_DARK_GRAY

int edhw_init()
{
  term_clrscr();
  term_gotoxy( 0, 0 );
  term_set_color( TEXTCOL, TERM_COL_BLACK );
  return 1;
}

int edhw_getkey()
{
  return term_getch( TERM_INPUT_WAIT );
}

void edhw_writechar( char c )
{
  term_putch( c );  
}

void edhw_writetext( const char* text )
{
  term_putstr( text, strlen( text ) );
}

void edhw_invertcols( int flag )
{
  if( flag )
    term_set_color( TERM_COL_BLACK, TEXTCOL );
  else
    term_set_color( TEXTCOL, TERM_COL_BLACK );
}

void edhw_gotoxy( int x, int y )
{
  term_gotoxy( x, y );
}

void edhw_setcursor( int type )
{
}

void edhw_clrscr()
{
  term_clrscr();
  term_gotoxy( 0, 0 );
}

void edhw_longline( int enable )
{
  if( enable )
    term_set_color( TERM_COL_WHITE, TERM_COL_DONT_CHANGE );
  else
    term_set_color( TEXTCOL, TERM_COL_DONT_CHANGE );
}

