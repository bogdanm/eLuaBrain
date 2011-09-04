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
#include "help.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EDITOR_MAIN_FILE
#include "edvars.h"

#define EDITOR_EXIT_CODE      0xFF
#define EDITOR_FATAL_CODE     ( -1 )

// ****************************************************************************
// Private functions and helpers

extern int docall( lua_State *L, int narg, int clear );

// Helper: print the error from a Lua state
static void editorh_print_lua_error( lua_State *L, void *buf )
{
  edhw_gotoxy( 0, 0 );
  edhw_writetext( "ERROR!\n\n" );
  edhw_writetext( lua_tostring( L, -1 ) );
  edhw_getkey();
  edhw_init();
  if( L )
    lua_close( L );
  if( buf )
    free( buf );
  edutils_show_screen();
}

// Helper: print a fatal error message
static void editorh_print_fatal_error( const char *title )
{
  edhw_msg( "A fatal error has occured, the editor will be closed", EDHW_MSG_ERROR, title );
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
        if( /*lua_pcall( L, 0, LUA_MULTRET, 0 )*/ docall( L, 0, 1 ) == 0 )
        {
          edhw_writetext( "Press any key to return to the editor" );
          edhw_getkey();
          edhw_init();
          lua_close( L );
          free( buf );
          edutils_show_screen();
        }
        else
          editorh_print_lua_error( L, buf );
      }
      else
        editorh_print_lua_error( L, buf );
    }
    else
    {
      edhw_msg( "Not enough memory", EDHW_MSG_ERROR, NULL );
      free( buf );
    }
  }
  else
    edhw_msg( "Not enough memory", EDHW_MSG_ERROR, NULL );
}

// Save the file - helper function
static int editorh_save_file( const char *fname, int show_confirmation )
{
  FILE *fp;
  int c;

  if( !fname || strlen( fname ) == 0 )
  {
    edhw_msg( "No file specified", EDHW_MSG_ERROR, NULL );
    return 0;
  }
  if( ( fp = fopen( fname, "wb" ) ) == NULL )
  {
    edhw_msg( "Unable to open file", EDHW_MSG_ERROR, NULL );
    return 0;
  }
  for( c = 0; c < ed_crt_buffer->file_lines; c ++ )
    if( fprintf( fp, "%s\n", ed_crt_buffer->lines[ c ] ) != strlen( ed_crt_buffer->lines[ c ] ) + 1 )
    {
      edhw_msg( "Error writing to file", EDHW_MSG_ERROR, NULL );
      fclose( fp ); // [TODO] remove file ?
      return 0;
    }
  fclose( fp );
  edutils_set_flag( ed_crt_buffer, EDFLAG_DIRTY, 0 );
  edutils_display_status();
  if( show_confirmation )
    edhw_msg( "File saved", EDHW_MSG_INFO, NULL );
  return 1;
}

// Save the file
static void editor_save_file()
{
  editorh_save_file( ed_crt_buffer->fpath, 1 );
}

// "Save as", returns -1 on allocation error
static int editor_saveas_file()
{
  char *line = edhw_read( "Save as", "Enter file name", 32, edutils_fname_validator );
  int res;

  if( !line )
    return 1;
  res = edalloc_set_fname( ed_crt_buffer, line );
  free( line );
  if( res )
    editorh_save_file( ed_crt_buffer->fpath, 1 );
  return res ? 1 : EDITOR_FATAL_CODE;
}

// Go to line
static void editor_goto_line()
{
  char *line;
  int newl;

  if( edutils_is_flag_set( ed_crt_buffer, EDFLAG_SELECT ) )
    return;
  line = edhw_read( "Go to line", "Enter line number", 6, edutils_number_validator );
  if( !line )
    return;
  newl = atoi( line );
  free( line );
  if( newl < 0 || newl > ed_crt_buffer->file_lines )
  {
    edhw_msg( "Invalid line number", EDHW_MSG_ERROR, NULL );
    return;
  }
  edmove_goto_line( newl );
}

// Show help page
static void editor_help()
{
  term_clrscr();
  term_set_cursor( TERM_CURSOR_OFF );
  printf( "Special keys\n" );
  printf( "------------\n" );
  printf( "  Key       | Action        \n" );
  printf( "  ==========================================================\n" );
  printf( "  F1        | Shows this help page\n" );
  printf( "  CTRL+F1   | Enters the API help mode\n" );
  printf( "  F2        | Save current file\n" );
  printf( "  CTRL+F2   | Save current file under a different name\n" );
  printf( "  F4        | Starts block selection mode (see next section)\n" );
  printf( "  CTRL+F4   | Clears the block buffer (see next section)\n" );
  printf( "  CTRL+V    | Pastes the block buffer\n" );
  printf( "  F5        | Run the current file\n" );
  printf( "  F7        | Go to the specified line\n" );
  printf( "  F10       | Exit from the editor\n" );
  printf( "  CTRL+Y    | Deletes the current line\n" );
  printf( "  CTRL+E    | Delete to the end of the line\n" );
  printf( "  CTRL+B    | Delete to the beginning of the line\n" );
  printf( "  ==========================================================\n" );  
  printf( "\nCopy and paste\n" );
  printf( "--------------\n" );
  printf( "The editor has limited support for copying and pasting blocks of text. " );
  printf( "Press F4 to enter block selection mode. In this mode the editor will highlight the lines that are selected. Press F4 again to end block selection mode. " );
  printf( "After pressing F4 the second time, the selected block of text is kept in memory in the block buffer. " );
  printf( "To paste it, press CTRL+V. This will also clear the block buffer. " );
  printf( "To clear the block buffer without pasting press CTRL+F4. " );
  printf( "While selecting a block, the 'go to line' command (F7) is disabled.\n" );
  printf( "\nPress any key to return to the editor." );
  term_getch( TERM_INPUT_WAIT );
  edhw_init();
  edutils_show_screen();
}

// Show API help interface
#define EDITOR_MAX_HELP_TOPIC 40
static void editor_apihelp()
{
  char input[ EDITOR_MAX_HELP_TOPIC + 1 ];
  int c, cx, cy, esc, entered;

  term_clrscr();
  term_set_mode( TERM_MODE_COLS );
  term_enable_paging( TERM_PAGING_ON );
  help_init( HELP_FILE_NAME ); 
  printf( "API help mode\n" );
  printf( "  Enter a topic to get help on, this can be a module name or a function name.\n" );
  printf( "  If nothing is entered, a list of all the available modules will be shown.\n" );
  printf( "  Press ESC at the prompt to return to the editor.\n\n" );
  while( 1 )
  {
    printf( "Enter topic: " );
    cx = term_get_cx();
    cy = term_get_cy();
    input[ 0 ] = '\0';
    entered = esc = 0;
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
      {
        printf( "\n" );
        break;
      }
      else if( c == KC_ESC ) // Escape
      {
        esc = 1;
        break;
      }
      else if( c < TERM_FIRST_KEY && strlen( input ) < EDITOR_MAX_HELP_TOPIC ) // regular ASCII char, ignore everything else
      {
        term_putch( c );
        term_gotoxy( ++cx, cy );
        input[ entered ++ ] = c;
        input[ entered ] = '\0';
      }
    }
    if( esc )
      break;
    help_help( input );
  }
  term_enable_paging( TERM_PAGING_OFF );
  term_set_mode( TERM_MODE_ASCII );
  help_close();
  edhw_init();
  edutils_show_screen();
}

// Exit from editor
static int editor_exit()
{
  int res;

  // Ask the user for confirmation
  if( ( res = edhw_dlg( "Are you sure you want to exit?", EDHW_DLG_YES | EDHW_DLG_NO, NULL ) ) == EDHW_DLG_NO )
    return 0;
  // If the file needs to be saved, ask the user
  if( edutils_is_flag_set( ed_crt_buffer, EDFLAG_DIRTY ) )
    if( ( res == edhw_dlg( "File not saved, save it now?", EDHW_DLG_YES | EDHW_DLG_NO, NULL ) ) == EDHW_DLG_YES )
    {
      if( ed_crt_buffer->fpath )
        editor_save_file();
      else
        editor_saveas_file();
    }
  edalloc_free_buffer( ed_crt_buffer );
  edalloc_deinit();
  term_reset();
  return EDITOR_EXIT_CODE;
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
  ed_firstsel = ed_lastsel = -1;
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
      printf( "FATAL ERROR IN EDITOR!!!!!\n" );
    }
    // Now check Fx commands
    res = 0;
    switch( c )
    {
      case KC_F1:
        editor_help();
        break;

      case KC_CTRL_F1:
        editor_apihelp();
        break;

      case KC_F5:
        editor_run();
        break;

      case KC_F2:
        if( ed_crt_buffer->fpath )
          editor_save_file();
        else
           res = editor_saveas_file();
        break;

      case KC_CTRL_F2:
        res = editor_saveas_file();
        break;

      case KC_F7:
        editor_goto_line();
        break;

      case KC_F10:
        res = editor_exit();
        break;
    }
    if( res == EDITOR_EXIT_CODE )
      break;
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
      printf( "FATAL ERROR IN EDITOR!!!!!\n" );
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

