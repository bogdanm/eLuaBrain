// Editor movement functions

#include "type.h"
#include "editor.h"
#include "edmove.h"
#include "edvars.h"
#include "term.h"
#include "edhw.h"
#include "edutils.h"
#include <ctype.h>

// ----------------------------------------------------------------------------
// Various helpers

// Check cursor position (and reposition if needed)
static void edmove_cursor_check()
{
  const char* pline = edutils_line_get( ed_startline + ed_cursory );

  if( ed_userstartx + ed_userx > edutils_get_actsize( pline ) ) // cursor is outside line, so reposition it
    edmove_set_cursorx( 0 );
  else
  {
    if( ed_startx != ed_userstartx )
    {
      ed_startx = ed_userstartx;
      edutils_show_screen();
    }
    ed_cursorx = ed_userx;
    edhw_gotoxy( ed_cursorx, ed_cursory );
  }
}

// Save current horizontal position (as requested by the user)
void edmove_save_cursorx()
{
  ed_userx = ed_cursorx;
  ed_userstartx = ed_startx;
}

// ****************************************************************************
// Actual movement functions

// Set the 'x' position properly (adjusting ed_startx if needed)
void edmove_set_cursorx( int x )
{
  int newstart;

  ed_cursorx = x;
  if( ed_cursorx >= TERM_COLS )
  {
    ed_cursorx = TERM_COLS - 1;
    newstart = x - ed_cursorx;
  }
  else
    newstart = 0;
  if( newstart != ed_startx )
  {
    ed_startx = newstart;
    edutils_show_screen();
  } 
  edhw_gotoxy( ed_cursorx, ed_cursory );
}

static void edmove_key_up()
{
  if( ed_crt_buffer->file_lines == 0 )
    return;
  if( ed_cursory == 0 )
  {
    if( ed_startline > 0 )
    {
      ed_startline --;
      edutils_show_screen();
    }
  }
  else
    ed_cursory --;
  edutils_display_status();
  edmove_cursor_check();
}

static void edmove_key_down()
{
  if( ed_crt_buffer->file_lines == 0 )
    return;
  if( ed_cursory == 0 )
  {
    if( ed_crt_buffer->file_lines > 1 )
      ed_cursory ++;
  }
  else if( ed_cursory == ed_nlines - 1 )
  {
    if( ed_nlines == EDITOR_LINES && ed_startline + EDITOR_LINES < ed_crt_buffer->file_lines )
    {
      ed_startline ++;
      edutils_show_screen();
    }
  }
  else
    ed_cursory ++;
  edutils_display_status();
  edmove_cursor_check();
}

static void edmove_key_pageup()
{
  if( ed_crt_buffer->file_lines == 0 )
    return;
  if( ed_startline == 0 )
    ed_cursory = 0;
  else
  {
    ed_startline = EMAX( 0, ed_startline - EDITOR_LINES );
    edutils_show_screen();
  }
  edutils_display_status();
  edmove_cursor_check();
}

static void edmove_key_pagedown()
{
  if( ed_crt_buffer->file_lines == 0 )
    return;
  if( ed_startline + ed_nlines == ed_crt_buffer->file_lines )
    ed_cursory = ed_nlines - 1;
  else
  {
    ed_startline = EMIN( ed_startline + EDITOR_LINES, ed_crt_buffer->file_lines - EDITOR_LINES );
    edutils_show_screen();
  }
  edutils_display_status();
  edmove_cursor_check();
}

static void edmove_key_left()
{
  if( ed_crt_buffer->file_lines == 0 )
    return;
  if( ed_cursorx == 0 )
  {
    if( ed_startx > 0 )
    {
      ed_startx --;
      edutils_show_screen();
    }
  }
  else
    ed_cursorx --;
  edutils_display_status();
}

static void edmove_key_right()
{
  const char *pline;

  if( ed_crt_buffer->file_lines == 0 )
    return;
  pline = edutils_line_get( ed_startline + ed_cursory ); 
  if( ed_startx + ed_cursorx == edutils_get_actsize( pline ) )
    return;
  if( ed_cursorx == TERM_COLS - 1 )
  {
    ed_startx ++;
    edutils_show_screen();
  }
  else
    ed_cursorx ++;
  edutils_display_status();
}

static void edmove_key_home()
{
  int firstc = 0;
  const char *pline;

  if( ed_crt_buffer->file_lines == 0 || ed_startx + ed_cursorx == 0 )
    return;
  // First press on 'home': go to the first non-space char on the line
  // Second press on 'home': go to the beginning of the line
  // Look for the first non-space char in the line
  pline = edutils_line_get( ed_startline + ed_cursory );
  while( *pline && isspace( *pline ) )
  {
    pline ++;
    firstc ++;
  }
  if( firstc == ed_startx + ed_cursorx )
    firstc = 0;
  edmove_set_cursorx( firstc );
}

void edmove_key_end()
{
  const char* pline;

  if( ed_crt_buffer->file_lines == 0 )
    return;
  pline = edutils_line_get( ed_startline + ed_cursory );
  ed_cursorx = edutils_get_actsize( pline );
  if( ed_cursorx > TERM_COLS - 1 )
  {
    ed_cursorx = TERM_COLS - 1;
    ed_startx = edutils_get_actsize( pline ) - ed_cursorx;
    edutils_show_screen();
  }
  else if( ed_startx > 0 )
  {
    ed_startx = 0;
    edutils_show_screen();
  }
  edutils_display_status();
}

// Take care of a movement key, returns 1 if the key is indeed a movement key
// or 0 otherwise
int edmove_handle_key( int c )
{
  switch( c )
  {
    case KC_UP:
      edmove_key_up();
      break;

    case KC_DOWN:
      edmove_key_down();
      break;

    case KC_PAGEUP:
      edmove_key_pageup();
      break;

    case KC_PAGEDOWN:
      edmove_key_pagedown();
      break;

    case KC_LEFT:
      edmove_key_left();
      edmove_save_cursorx();
      break;

    case KC_RIGHT:
      edmove_key_right();
      edmove_save_cursorx();
      break;

    case KC_HOME:
      edmove_key_home();
      edmove_save_cursorx();
      break;

    case KC_END:
      edmove_key_end();
      edmove_save_cursorx();
      break;

    default:
      return 0;
  }
  return 1;
}

