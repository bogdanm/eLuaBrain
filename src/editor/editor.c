// Editor implementation

#include "editor.h"
#include "edalloc.h"
#include "edhw.h"
#include "type.h"
#include "term.h"
#include "edutils.h"
#include "edmove.h"
#include "ededit.h"
#include "term.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EDITOR_MAIN_FILE
#include "edvars.h"

// ****************************************************************************
// Private function

static void editor_run()
{
  int c, res;
  char *buf, *crt;
  lua_State *L;

  // Step 1: count the total buffer size
  for( res = c = 0; c < ed_crt_buffer->file_lines; c ++ )
    res += strlen( ed_crt_buffer->lines[ c ] ) + 1;
  // Step 2: alloc memory and write data there
  if( buf = ( char* )malloc( res ) )
  {
    crt = buf;
    for( c = 0; c < ed_crt_buffer->file_lines; c ++ )
    {
      strcpy( crt, ed_crt_buffer->lines[ c ] );
      strcat( crt, "\n" );
      crt += strlen( crt );
    }
    // Step 3: create a Lua state and run this
    L = lua_open();
    luaL_openlibs( L );
    term_clrscr();
    term_gotoxy( 0, 0 );
    term_set_color( TERM_COL_DEFAULT, TERM_COL_DEFAULT ); 
    luaL_loadbuffer( L, buf, res, "editor" );
    lua_pcall( L, 0, LUA_MULTRET, 0 );
    lua_close( L );
    free( buf );
    term_getch( TERM_INPUT_WAIT );
    edhw_init();
    edutils_show_screen();
  }
}

// ****************************************************************************
// Public interface

// Initialize the editor with the given file
int editor_init( const char *fname )
{
  if( !edhw_init() )
    return 0;
  if( !edalloc_init() )
    return 0;
  if( ( ed_crt_buffer = edalloc_buffer_new( fname ) ) == NULL )
    return 0;
  ed_cursorx = ed_cursory = 0;
  ed_startx = ed_startline = 0;
  ed_userx = ed_userstartx = 0;
  edhw_invertcols( 0 );
  return 1;
}

int editor_mainloop()
{
  int c, res;

  edutils_show_screen();
  while( 1 ) // Read and interpret keys
  {
    c = edhw_getkey();
    // Check for movement keys first
    if( edmove_handle_key( c ) )
      continue;
    // Then for editing keys
    if( ( res = ededit_handle_key( c ) ) == 1 )
      continue;
    if( res == -1 )
    {
      // [TODO] handle fatal error here!
    }
    // Now check "run"
    if( c == KC_F5 )
      editor_run();
  }
  return 0;
}

#ifdef EDITOR_STANDALONE
int main( int argc, char **argv )
{
  int c, res;

  if( !editor_init( argv[ 1 ] ) )
  {
    PRINTF( "ERROR: unable to initialize editor\n" );
    getch();
    endwin();
    return 0;
  }
  else
  {
    PRINTF( "Read %d lines\n", ed_crt_buffer->file_lines );
    getch();
  }
  edutils_show_screen();
  while( 1 ) // Read and interpret keys
  {
    c = edhw_getkey();
    // Check for movement keys first
    if( edmove_handle_key( c ) )
      continue;
    // Then for editing keys
    if( ( res = ededit_handle_key( c ) ) == 1 )
      continue;
    if( res == -1 )
    {
      // [TODO] handle fatal error here!
    }
    // Now check "save"
    if( c == KC_CTRL + 'S' ) 
    {
      FILE *fp = fopen( "edit.out", "wb" );
      for( c = 0; c < ed_crt_buffer->file_lines; c ++ )
        fprintf( fp, "%s\n", ed_crt_buffer->lines[ c ] );
      fclose( fp );
      break;
    }
  }
  endwin();
  return 0;
}
#endif

