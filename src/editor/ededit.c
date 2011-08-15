// Actual text editing functionality

#include "ededit.h"
#include "type.h"
#include "editor.h"
#include "term.h"
#include "edvars.h"
#include "edalloc.h"
#include "edutils.h"
#include "edmove.h"
#include "utils.h"
#include "edhw.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// *************************************************************************
// Local functions

// Add a string to a line
static int ededit_addstring( int lineid, int linepos, const char* pstr )
{
  char* pline = edutils_line_get( lineid );
  
  if( ( pline = edalloc_line_realloc( pline, strlen( pline ) + strlen( pstr ) + 1 ) ) == NULL )
    return -1;
  if( strlen( pline ) == linepos )
    strcat( pline, pstr );
  else
  {
    memmove( pline + linepos + strlen( pstr ), pline + linepos, strlen( pline ) - linepos + 1 );
    memcpy( pline + linepos, pstr, strlen( pstr ) );
  } 
  edutils_line_set( lineid, pline );
  return 1;
}

// Add a char to the current line
static int ededit_key_char( int c )
{
  char s[ 2 ];
  int lineid = ed_startline + ed_cursory;

  s[ 0 ] = c;
  s[ 1 ] = '\0';
  if( ededit_addstring( ed_startline + ed_cursory, ed_startx + ed_cursorx, s ) == -1 )
    return -1;
  edmove_set_cursorx( ed_startx + ed_cursorx + 1 );
  edutils_line_display( ed_cursory, lineid );
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
  edutils_display_status();
  return 1;
}

// Remove a char (handle 'backspace')
static int ededit_key_backspace()
{
  int linepos = ed_startx + ed_cursorx;
  int lineid = ed_startline + ed_cursory;
  char *pline = edutils_line_get( lineid );

  if( linepos == 0 ) // this is the first column, so join this line with the one above
  {
    if( lineid == 0 )
      return 1;
    linepos = strlen( edutils_line_get( lineid - 1 ) );
    if( ededit_addstring( lineid - 1, linepos, pline ) == -1 )
      return -1;
    edalloc_buffer_remove_line( ed_crt_buffer, lineid );
    //if( ( ed_crt_buffer->file_lines == lineid && ed_startline > 0 ) || ed_cursory == 0 )
    if( ed_cursory == 0 || ( ed_startline > 0 && ed_startline + EDITOR_LINES > ed_crt_buffer->file_lines ) )
      ed_startline --;
    else
      ed_cursory --;
    edmove_set_cursorx( linepos );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    if( ed_nlines < EDITOR_LINES && ed_startline == 0 ) 
      edutils_set_flag( ed_crt_buffer, EDFLAG_DEL_LAST_LINE, 1 );
    edutils_show_screen();
  }
  else // not the first column, simply remove a char and update the display
  {
    linepos --;
    memmove( pline + linepos, pline + linepos + 1, ( strlen( pline ) - linepos ) * sizeof( char ) ); 
    pline = edalloc_line_realloc( pline, strlen( pline ) + 1 );
    edutils_line_set( lineid, pline );
    edmove_set_cursorx( ed_startx + ed_cursorx - 1 );
    edutils_line_display( ed_cursory, lineid );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    edutils_display_status();
  }
  return 1;
}

// Remove a char to the right of the cursor (handle 'del')
static int ededit_key_del()
{
  int linepos = ed_startx + ed_cursorx;
  int lineid = ed_startline + ed_cursory;
  char *pline = edutils_line_get( lineid );

  if( linepos == strlen( pline ) ) // this is the last column, so join this line with the one below
  {
    if( lineid == ed_crt_buffer->file_lines - 1 ) // nothing to do on the last line
      return 1;
    if( ededit_addstring( lineid, linepos, edutils_line_get( lineid + 1 ) ) == -1 )
      return -1;
    edalloc_buffer_remove_line( ed_crt_buffer, lineid + 1 );
    if( ed_startline > 0 && ed_startline + EDITOR_LINES > ed_crt_buffer->file_lines )
    {
      ed_startline --;
      ed_cursory ++;
    }
    edmove_set_cursorx( linepos );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    edutils_show_screen();
  }
  else if( strlen( pline ) > 0 ) // not in the last column, simply remove a char and update the display
  {  
    memmove( pline + linepos, pline + linepos + 1, ( strlen( pline ) - linepos ) * sizeof( char ) );
    pline = edalloc_line_realloc( pline, strlen( pline ) + 1 );
    edutils_line_set( lineid, pline );
    edutils_line_display( ed_cursory, lineid );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    edutils_display_status();
  }
  return 1;
}

// Handle the 'enter' key
static int ededit_key_enter()
{
  int linepos = ed_startx + ed_cursorx;
  int lineid = ed_startline + ed_cursory;
  char *oldline = edutils_line_get( lineid ), *newline, *p;
  int indentation, i;

  // Get the line indentation
  p = oldline;
  while( *p == ' ' )
    p ++;
  indentation = p - oldline;
  // Split the line in two pieces and make room in the line buffer
  // This code works in all situations (cursor at the beginning of line, cursor at the end of line,
  // cursor between the beginning and the end of line)
  if( ( newline = edalloc_line_malloc( strlen( oldline + linepos ) + indentation + 1 ) ) == NULL )
    return -1;
  for( i = 0; i < indentation; i ++ )
    newline[ i ] = ' ';
  strcpy( newline + indentation, oldline + linepos );
  if( ( oldline = edalloc_line_realloc( oldline, linepos + 1 ) ) == NULL )
    return -1;
  oldline[ linepos ] = '\0';
  if( edalloc_buffer_add_line( ed_crt_buffer, lineid, oldline ) == 0 )
    return -1;
  edutils_line_set( lineid + 1, newline );
  edmove_set_cursorx( indentation );
  edmove_save_cursorx();
  if( ed_cursory == EDITOR_LINES - 1 )
    ed_startline ++;
  else
    ed_cursory ++;
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
  edutils_show_screen();
  return 1;
}

// Delete the current line
static int ededit_delline()
{
  int lineid = ed_startline + ed_cursory;
  char *pline = edutils_line_get( lineid );

  if( ed_crt_buffer->file_lines == 1 )
  {
    // If we have a single line we don't delete it, we replace it with an empty line instead
    if( strlen( pline ) > 0 )
    {
      if( ( pline = edalloc_line_realloc( pline, 1 ) ) == NULL )
        return -1;
      pline[ 0 ] = '\0';
      edutils_line_set( 0, pline );
    }
    else
      return 1;
  }
  else
  {
    edhw_clearline( UMIN( EDITOR_LINES - 1, ed_crt_buffer->file_lines - 1 ) );
    edalloc_buffer_remove_line( ed_crt_buffer, lineid );
  }
  if( ed_crt_buffer->file_lines == lineid )
  {
    if( ed_startline > 0 )
      ed_startline --;
    else
      ed_cursory --;
  }
  edmove_set_cursorx( 0 );
  edmove_save_cursorx();
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
  edutils_show_screen();
  return 1;
}

// Delete to the end of the current line
static int ededit_deltoeol()
{
  int lineid = ed_startline + ed_cursory;
  int linepos = ed_startx + ed_cursorx;
  char *pline = edutils_line_get( lineid );

  if( strlen( pline ) > 0 && linepos != strlen( pline ) )
  {
    if( ( pline = edalloc_line_realloc( pline, linepos + 1 ) ) == NULL )
      return -1;
    pline[ linepos ] = '\0';
    edutils_line_set( lineid, pline );
    edutils_line_display( ed_cursory, lineid );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    edmove_set_cursorx( linepos );    
  }
  return 1;
}

// Delete to the beginning of the current line
static int ededit_deltobol()
{
  int lineid = ed_startline + ed_cursory;
  int linepos = ed_startx + ed_cursorx;
  char *pline = edutils_line_get( lineid );
  
  if( strlen( pline ) > 0 && linepos != 0 )
  {
    memmove( pline, pline + linepos, strlen( pline + linepos ) + 1 );
    if( ( pline = edalloc_line_realloc( pline, strlen( pline + linepos ) + 1 ) ) == NULL )
      return -2;
    edutils_line_set( lineid, pline );
    edutils_line_display( ed_cursory, lineid );
    edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
    edmove_set_cursorx( 0 );
  }
  return 1;
}

// Copy a block to the current cursor position
static int ededit_pasteblock()
{
  int i;

  if( ed_sellines == NULL )
    return 1;
  // Set the lines in the block
  for( i = ed_lastsel; i >= ed_firstsel; i -- )
    if( edalloc_buffer_add_line( ed_crt_buffer, ed_startline + ed_cursory, ed_sellines[ i - ed_firstsel ] ) == 0 )
    {
      edalloc_clear_selection( ed_crt_buffer );
      return -1;
    }
  edalloc_reset_used_selection( ed_crt_buffer );
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 1 );
  edmove_set_cursorx( 0 );
  edutils_show_screen();
  return 1;
}

// *************************************************************************
// Public interface

// Handle a text editing key, return 1 if it actually was a text editing key
// or 0 otherwise
// Return '-1' if a fatal error occured (for example out of memory)
int ededit_handle_key( int c )
{
  int res;

  if( isprint( c ) )
    return ededit_key_char( c );
  switch( c )
  {
    case KC_BACKSPACE:
      return ededit_key_backspace();

    case KC_DEL:
      return ededit_key_del();

    case KC_TAB:
      res = ededit_key_char( ' ' );
      if( res )
        res = ededit_key_char( ' ' );
      return res;

    case KC_ENTER:
      return ededit_key_enter();

    case KC_CTRL_Y:
      return ededit_delline();

    case KC_CTRL_E:
      return ededit_deltoeol();

    case KC_CTRL_B:
      return ededit_deltobol();

    case KC_CTRL_V:
      return ededit_pasteblock();
  }
  return 0;
}

