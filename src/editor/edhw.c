// Editor hardware related functions

#include "edhw.h"
#include "editor.h"
#include "type.h"
#include "term.h"
#include "platform_conf.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define TEXTCOL       TERM_COL_DARK_GRAY
#define TBOX_WIDTH    26
#define TBOX_HEIGHT   3
#define EBOX_WIDTH    26
#define EBOX_HEIGHT   4
#define DBOX_WIDTH    26
#define DBOX_HEIGHT   4

int edhw_init()
{
  term_reset();
  term_set_color( TEXTCOL, TERM_COL_BLACK );
  return 1;
}

int edhw_getkey( void )
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

void edhw_clearline( int line )
{
  int x = term_get_cx(), y = term_get_cy();

  term_gotoxy( 0, line );
  term_clreol();
  term_gotoxy( x, y );
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

void edhw_selectedline( int enable )
{
  if( enable )
    term_set_color( TERM_COL_BLACK, TERM_COL_WHITE );
  else
    term_set_color( TEXTCOL, TERM_COL_BLACK );
}

void edhw_msg( const char *text, int type, const char *title )
{
  int fgcol = TERM_COL_LIGHT_BLUE, bgcol = TERM_COL_DARK_GRAY;
  void *pbox;
  int prevfg, prevbg;
  int bx, by;
  int width = UMAX( TBOX_WIDTH, strlen( text ) + 4 );
  int prevcx = term_get_cx(), prevcy = term_get_cy();
 
  if( title == NULL )
  {
    if( type == EDHW_MSG_ERROR )
    {
      fgcol = TERM_COL_LIGHT_RED;
      title = "Error";
    }
    else if( type == EDHW_MSG_INFO )
      title = "Info";
    else
    {
      fgcol = TERM_COL_LIGHT_YELLOW;
      title = "Warning";
    }
  }
  term_get_color( &prevfg, &prevbg );
  term_set_color( fgcol, bgcol );
  term_set_cursor( TERM_CURSOR_OFF );
  pbox = term_box( bx = ( TERM_COLS - width ) >> 1, by = ( TERM_LINES - TBOX_HEIGHT ) >> 1, width, TBOX_HEIGHT, title, TERM_BOX_FLAG_BORDER | TERM_BOX_FLAG_RESTORE );
  term_cstrxy( bx + 2, by + 1, text );
  term_getch( TERM_INPUT_WAIT );
  term_close_box( pbox );
  term_set_color( prevfg, prevbg );
  term_set_cursor( TERM_CURSOR_BLOCK );
  term_gotoxy( prevcx, prevcy );
}

char* edhw_read( const char *title, const char *text, unsigned maxlen, p_ed_validate validator )
{
  char *input;
  void *pbox;
  int prevfg, prevbg;
  int width = UMAX( EBOX_WIDTH, UMAX( strlen( text ), maxlen ) + 4 );
  int height = strlen( text ) > 0 ? EBOX_HEIGHT : EBOX_HEIGHT - 1;
  int bx, by, cy = strlen( text ) > 0 ? 2 : 1, cx;
  int entered = 0, c, vflag;
  int prevcx = term_get_cx(), prevcy = term_get_cy();

  if( ( input = malloc( maxlen + 1 ) ) == NULL )
    return NULL;
  term_get_color( &prevfg, &prevbg );
  term_set_color( TERM_COL_LIGHT_BLUE, TERM_COL_DARK_GRAY );
  pbox = term_box( bx = ( TERM_COLS - width ) >> 1, by = ( TERM_LINES - height ) >> 1, width, height, title, TERM_BOX_FLAG_BORDER | TERM_BOX_FLAG_RESTORE );
  term_cstrxy( bx + 2, by + 1, text );
  term_gotoxy( cx = bx + 2, cy = cy + by );
  input[ 0 ] = '\0';
  while( 1 )
  {
    c = term_getch( TERM_INPUT_WAIT );
    if( c == KC_BACKSPACE )
    {
      if( entered > 0 )
      {
        term_gotoxy( cx - 1, cy );
        term_putch( ' ' );
        term_gotoxy( -- cx, cy );
        input[ --entered ] = '\0';
      }
    }
    else if( c == KC_ENTER ) // NL
      break;
    else if( c == KC_ESC ) // Escape
    {
      free( input );
      input = NULL;
      break;
    }
    else if( c < TERM_FIRST_KEY && strlen( input ) < maxlen) // regular ASCII char, ignore everything else
    {
      vflag = validator ? validator( input, c ) : 1;
      if( vflag )
      {
        term_putch( c );
        term_gotoxy( ++cx, cy );
        input[ entered ++ ] = c;
        input[ entered ] = '\0';
      }
    }
  }
  term_close_box( pbox );
  term_set_color( prevfg, prevbg );
  term_gotoxy( prevcx, prevcy );
  return input;
}

static u8 edhw_dlg_chars[] = { 'y', 'n', 'c', 'o' };
static const char* edhw_dlg_msgs[] = { "Yes", "No", "Cancel", "OK" };
int edhw_dlg( const char *text, int type, const char *title )
{
  int allowed[ EDHW_DLG_TOTAL ];
  unsigned i;
  int fgcol = TERM_COL_LIGHT_BLUE, bgcol = TERM_COL_DARK_GRAY;
  void *pbox;
  int prevfg, prevbg;
  int bx, by;
  int width;
  int prevcx = term_get_cx(), prevcy = term_get_cy();
  int btnlinewidth = 0, numbuttons = 0;

  for( i = 0; i < EDHW_DLG_TOTAL; i ++ )
    if( type & ( 1 << i ) )
    {
      allowed[ i ] = edhw_dlg_chars[ i ];
      btnlinewidth += strlen( edhw_dlg_msgs[ i ] ) + 1;
      numbuttons ++;
    }
    else
      allowed[ i ] = 0;
  if( title == NULL )
    title = "Confirmation";
  // Prepare line with button texts
  width = UMAX( DBOX_WIDTH, UMAX( strlen( text ), btnlinewidth ) + 4 );
  term_get_color( &prevfg, &prevbg );
  term_set_color( fgcol, bgcol );
  term_set_cursor( TERM_CURSOR_OFF );
  pbox = term_box( bx = ( TERM_COLS - width ) >> 1, by = ( TERM_LINES - DBOX_HEIGHT ) >> 1, width, DBOX_HEIGHT, title, TERM_BOX_FLAG_BORDER | TERM_BOX_FLAG_RESTORE );
  term_cstrxy( bx + 2, by + 1, text );
  // Write the line with allowed buttons
  term_gotoxy( bx + 2, by + 2 );
  for( i = 0; i < EDHW_DLG_TOTAL; i ++ )
    if( type & ( 1 << i ) )
    {
      term_set_color( TERM_COL_LIGHT_RED, bgcol );
      term_putstr( edhw_dlg_msgs[ i ], 1 );
      term_set_color( fgcol, bgcol );
      term_cstr( edhw_dlg_msgs[ i ] + 1 );
      if( numbuttons > 1 )
        term_cstr( "/" );
      numbuttons --;
    }
  // Now get input from user
  while( 1 )
  {
    bx = term_getch( TERM_INPUT_WAIT );
    for( i = 0; i < EDHW_DLG_TOTAL; i ++ )
      if( allowed[ i ] == bx || toupper( allowed[ i ] ) == bx )
        break;
    if( i < EDHW_DLG_TOTAL )
      break;
  }
  term_close_box( pbox );
  term_set_color( prevfg, prevbg );
  term_set_cursor( TERM_CURSOR_BLOCK );
  term_gotoxy( prevcx, prevcy ); 
  return 1 << i;
}

