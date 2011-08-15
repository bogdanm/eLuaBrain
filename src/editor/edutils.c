// General utilities

#include "editor.h"
#include "edutils.h"
#include "type.h"
#include "edvars.h"
#include "edhw.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "term.h"

// Helper: return "true" if the given line is selected, false otherwise
static int edutilsh_is_selected( int lineid )
{
  int first, last;

  if( !edutils_is_flag_set( ed_crt_buffer, EDFLAG_SELECT ) )
    return 0;
  first = ed_firstsel;
  last = ed_lastsel;
  if( first > last )
  {
    first = ed_lastsel;
    last = ed_firstsel;
  }
  return ( first <= lineid ) && ( lineid <= last );
}

// Get a line from the file
char* edutils_line_get( int id )
{
  return ed_crt_buffer->lines[ id ];
}

// Set a line from the file
void edutils_line_set( int id, char* pline )
{
  ed_crt_buffer->lines[ id ] = pline;
}

// Get the actual line of a text
// (this ignores '\r' and '\n' chars)
int edutils_get_actsize( const char* pdata )
{
  int l = strlen( pdata );

  while( l > 0 && ( pdata[ l - 1 ] == '\r' || pdata[ l - 1 ] == '\n' ) )
    l --;
  return l;
}

int edutils_is_flag_set( EDITOR_BUFFER* b, int flag )
{ 
  return ( b->flags & flag ) != 0;
}

void edutils_set_flag( EDITOR_BUFFER* b, int flag, int value )
{
  if( value == 0 )
    b->flags &= ~flag;
  else
    b->flags |= flag;
}

// Status line display
void edutils_display_status()
{
  unsigned i;
  char temp[ TERM_COLS + 1 ];

  edhw_invertcols( 1 );
  // Write current position
  temp[ 0 ] = temp[ TERM_COLS ] = '\0';
  snprintf( temp, TERM_COLS, " %c %s    Col: %d    Line: %d/%d     Buffer: %d line(s)", edutils_is_flag_set( ed_crt_buffer, EDFLAG_DIRTY ) ? '*' : ' ', 
            ed_crt_buffer->fpath ? ed_crt_buffer->fpath : "(none)",  ed_cursorx + ed_startx + 1, ed_cursory + ed_startline + 1, 
            ed_crt_buffer->file_lines, ed_sellines ? ed_lastsel - ed_firstsel + 1 : 0 );
  edhw_gotoxy( 0, TERM_LINES - 1 );
  edhw_writetext( temp );
  for( i = strlen( temp ); i < TERM_COLS; i ++ )
    edhw_writechar( ' ' );
  edhw_invertcols( 0 );
  edhw_gotoxy( ed_cursorx, ed_cursory );
}

// Display (part of a) line at a given location
void edutils_line_display( int scrline, int id )
{
  const char* pline = edutils_line_get( id );
  int nspaces = TERM_COLS, i;
  int longline = edutils_get_actsize( pline ) > TERM_COLS;

  edhw_gotoxy( 0, scrline );
  if( longline )
    edhw_longline( 1 );
  if( edutilsh_is_selected( id ) )
    edhw_selectedline( 1 );
  if( pline && ed_startx < strlen( pline ) )  
    for( i = 0; i < EMIN( strlen( pline ) - ed_startx, TERM_COLS ); i ++, nspaces -- )
      edhw_writechar( pline[ ed_startx + i ] );
  for( i = 0; i < nspaces; i ++ )
    edhw_writechar( ' ' ); 
  if( longline )
    edhw_longline( 0 );
  if( edutilsh_is_selected( id ) )
    edhw_selectedline( 0 );
}

// Display the current editor screen
void edutils_show_screen()
{
  unsigned i;

  ed_nlines = EDITOR_LINES;
  if( ed_startline + ed_nlines > ed_crt_buffer->file_lines )
    ed_nlines = ed_crt_buffer->file_lines - ed_startline;
  //edhw_clrscr();
  for( i = ed_startline; i < ed_startline + ed_nlines; i ++ )
    edutils_line_display( i - ed_startline, i );
  if( edutils_is_flag_set( ed_crt_buffer, EDFLAG_DEL_LAST_LINE ) )
  {
    edhw_gotoxy( 0, ed_nlines );
    for( i = 0; i < TERM_COLS; i ++ )
      edhw_writechar( ' ' );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DEL_LAST_LINE, 0 );
  }
  edutils_display_status();
}

// Input validator: number
int edutils_number_validator( const char *input, int c )
{
  return isdigit( c ) || ( strlen( input ) == 0 && c == '-' );
}

// Input validator: file name
int edutils_fname_validator( const char *input, int c )
{
  return isgraph( c );
}

