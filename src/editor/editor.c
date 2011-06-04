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
#include "edutils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EDITOR_MAIN_FILE
#include "edvars.h"

// ****************************************************************************
// Private functions and helpers

// Helper: print the error from a Lua state
static void editorh_print_lua_error( lua_State *L, void *buf )
{
  edhw_writetext( "\nERROR!\n\n" );
  edhw_writetext( lua_tostring( L, -1 ) );
  edhw_getkey();
  edhw_init();
  edutils_show_screen();
  if( L )
    lua_close( L );
  if( buf )
    free( buf );
}

// Run the program currently in the editor
static void editor_run()
{
  int c, res;
  char *buf = NULL, *crt;
  lua_State *L = NULL;

  // Step 1: count the total buffer size
  for( res = c = 0; c < ed_crt_buffer->file_lines; c ++ )
    res += strlen( ed_crt_buffer->lines[ c ] ) + 1;
  // Step 2: alloc memory and write data there
  if( ( buf = ( char* )malloc( res + 1 ) ) != NULL )
  {
    crt = buf;
    for( c = 0; c < ed_crt_buffer->file_lines; c ++ )
    {
      strcpy( crt, ed_crt_buffer->lines[ c ] );
      strcat( crt, "\n" );
      crt += strlen( crt );
    }
    // Step 3: create a Lua state and run this
    if( ( L = lua_open() ) != NULL )
    {
      term_reset();
      luaL_openlibs( L );
      if( luaL_loadbuffer( L, buf, res, "editor" ) == 0 )
      {
        if( lua_pcall( L, 0, LUA_MULTRET, 0 ) == 0 )
        {
          edhw_writetext( "Press any key to return to the editor" );
          edhw_getkey();
          edhw_init();
          edutils_show_screen();
          lua_close( L );
          free( buf );
        }
        else
          editorh_print_lua_error( L, buf );
      }
      else
        editorh_print_lua_error( L, buf );
    }
    else
    {
      edhw_msg( "Not enough memory", EDHW_MSG_ERROR );
      free( buf );
    }
  }
  else
    edhw_msg( "Not enough memory", EDHW_MSG_ERROR );
}

// Save the file - helper function
static int editorh_save_file( const char *fname )
{
  FILE *fp;
  int c;

  if( !fname || strlen( fname ) == 0 )
  {
    edhw_msg( "No file specified", EDHW_MSG_ERROR );
    return 0;
  }
  if( ( fp = fopen( fname, "wb" ) ) == NULL )
  {
    edhw_msg( "Unable to open file", EDHW_MSG_ERROR );
    return 0;
  }
  for( c = 0; c < ed_crt_buffer->file_lines; c ++ )
    if( fprintf( fp, "%s\n", ed_crt_buffer->lines[ c ] ) != strlen( ed_crt_buffer->lines[ c ] ) + 1 )
    {
      edhw_msg( "Error writing to file", EDHW_MSG_ERROR );
      fclose( fp ); // [TODO] remove file ?
      return 0;
    }
  fclose( fp );
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 0 );
  edutils_display_status();
  edhw_msg( "File saved", EDHW_MSG_INFO );
  return 1;
}

// Save the file
static void editor_save_file()
{
  editorh_save_file( ed_crt_buffer->fpath );
}

// "Save as"
static void editor_saveas_file()
{
  char *line = edhw_read( "Save as", "Enter file name", 32, edutils_fname_validator );

  editorh_save_file( line );
  free( line );
}

// Go to line
static void editor_goto_line()
{
  char *line = edhw_read( "Go to line", "Enter line number", 6, edutils_number_validator );
  int newl = atoi( line );

  free( line );
  if( newl < 0 || newl > ed_crt_buffer->file_lines )
  {
    edhw_msg( "Invalid line number", EDHW_MSG_ERROR );
    return;
  }
  edmove_goto_line( newl );
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
    else if( c == KC_F2 ) // "save"
      editor_save_file();
    else if( c == KC_CTRL_F2 ) // "save as"
      editor_saveas_file();
    else if( c == KC_F7 ) // "go to line" 
      editor_goto_line();
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

