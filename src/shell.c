// eLua shell

#include "shell.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "xmodem.h"
#include "version.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "platform.h"
#include "elua_net.h"
#include "devman.h"
#include "term.h"
#include "buf.h"
#include "remotefs.h"
#include "eluarpc.h"
#include "linenoise.h"
#include "stm32f10x.h"
#include "platform_conf.h"
#include "editor.h"
#include "common.h"
#ifdef BUILD_SHELL

#define EE_I2C_ADDR               0xA0
#define EE_I2C_NUM                0
#define EE_PAGE_SIZE              64

// Shell alternate ' ' char
#define SHELL_ALT_SPACE           '\x07'

// EOF is different in UART mode and TCP/IP mode
#ifdef BUILD_CON_GENERIC
  #define SHELL_EOF_STRING        "CTRL+Z"
#else
  #define SHELL_EOF_STRING        "CTRL+D"
#endif

// Shell command handler function
typedef void( *p_shell_handler )( char* args );

// Command/handler pair structure
typedef struct
{
  const char* cmd;
  p_shell_handler handler_func;
} SHELL_COMMAND;

// Shell data
static char* shell_prog;

// Callback function for FS iterator
typedef int ( *p_shell_iterator_cb )( const char *fsname, struct dm_dirent *, void *f, int iterno );

// ****************************************************************************
// Shell helpers

// Generic FS iterator for 'ls', 'cp' and 'rm'
static int shellh_pattern_iterator( const char *pattern, p_shell_iterator_cb cb, void *f )
{
  const char *pfname;
  int ptype = cmn_get_path_type( pattern, &pfname );
  DM_DIR *d;
  struct dm_dirent *ent;
  char *pdevname;
  int i = 0;

  if( ptype == PATH_TYPE_INVALID || ptype == PATH_TYPE_FS )
    return 0;
  if( ( pdevname = strndup( pattern, pfname - pattern - 1 ) ) == NULL )
  {
    printf( "Not enough memory.\n" );
    return 0;
  }
  if( ( d = dm_opendir( pdevname ) ) != NULL )// iterate and call only matching files
  {
    cb( pdevname, NULL, f, -1 );
    while( ( ent = dm_readdir( d ) ) != NULL )
      if( cmn_match_fname( ent->fname, pfname ) )
        if( cb( pdevname, ent, f, i ++ ) == 0 ) // exit iteration indication
          break;
    cb( pdevname, NULL, f, i ); // last iteration
    dm_closedir( d );
  }
  printf( TERM_RESET_COL ); // just in case 
  free( pdevname );
  return 1;
}

// Helper: transform a FS name (such as "/rom") into a wildcard pattern ("/rom/*")
static char* shellh_pattern_from_fs( const char *pfs )
{
  char *ppattern;

  if( ( ppattern = malloc( strlen( pfs ) + 3 ) ) == NULL )
  {
    printf( "Not enough memory.\n" );
    return NULL;
  }
  strcpy( ppattern, pfs );
  if( ppattern[ strlen( ppattern ) - 1 ] != '/' )
    strcat( ppattern, "/" );
  strcat( ppattern, "*" );
  return ppattern;
}

// Print a colorized "[Yes/No/eXit]" prompt
static void shellh_print_action_prompt()
{
  printf( "[%sY%ses/%sN%so/e%sX%sit] ? %s", TERM_FGCOL_LIGHT_YELLOW, TERM_FGCOL_LIGHT_BLUE,
    TERM_FGCOL_LIGHT_YELLOW, TERM_FGCOL_LIGHT_BLUE,
    TERM_FGCOL_LIGHT_YELLOW, TERM_FGCOL_LIGHT_BLUE, TERM_RESET_COL );
}

// ****************************************************************************
// Shell functions

// ----------------------------------------------------------------------------
// 'help' handler
static void shell_help( char* args )
{
  args = args;
  printf( "Shell commands:\n" );
  printf( "  exit        - exit from this shell\n" );
  printf( "  help        - print this help\n" );
  printf( "  ls [pattern] - lists filesystems files and sizes (optionally matching 'pattern')\n" );
  printf( "  cat or type - lists file contents\n" );
  printf( "  lua [args]  - run Lua with the given arguments\n" );
  printf( "  recv [path] - receive a file via XMODEM, if there is a path, save"
          "                there, otherwise run it.");
  printf( "  cp <src> <dst> [-b] [-c] [-o] - copy source file 'src' to 'dst'\n" );
  printf( "     -b: backup mode (prepend the destination file name with '_b').\n" );
  printf( "     -c: ask for confirmation before copying a file.\n" );
  printf( "     -o: overwrite destination files without confirmation.\n" );
  printf( "  ee <file>   - dump file to the EEPROM connected on I2C1\n" );
  printf( "  edit <file> - edits the given file\n" );
  printf( "  ver         - print eLua version\n" );
  printf( "  rm <file> [-f] - removes the file, use '-f' to supress confirmation\n" );
}

// ----------------------------------------------------------------------------
// 'lua' handler
static void shell_lua( char* args )
{
  int nargs = 0;
  char* lua_argv[ SHELL_MAX_LUA_ARGS + 2 ];
  char *p, *prev, *temp;

  lua_argv[ 0 ] = "lua";
  // Process "args" if needed
  if( *args )
  {
    prev = args;
    p = strchr( args, ' ' );
    while( p )
    {
      if( nargs == SHELL_MAX_LUA_ARGS )
      {
        printf( "Too many arguments to 'lua' (maxim %d)\n", SHELL_MAX_LUA_ARGS );
        return;
      }
      *p = 0;
      lua_argv[ nargs + 1 ] = temp = prev;
      nargs ++;
      prev = p + 1;
      p = strchr( p + 1, ' ' );
      // If the argument is quoted, remove the quotes and transform the 'alternate chars' back to space
      if( *temp == '\'' || *temp == '"' )
      {
        temp ++;
        lua_argv[ nargs ] = temp;
        while( *temp )
        {
          if( *temp == SHELL_ALT_SPACE )
            *temp = ' ';
          temp ++;
        }
        *( temp - 1 ) = '\0';
      }
    }
  }
  lua_argv[ nargs + 1 ] = NULL;
  printf( "Press " SHELL_EOF_STRING " to exit Lua\n" );
  lua_main( nargs + 1, lua_argv );
  clearerr( stdin );
}

// ----------------------------------------------------------------------------
// 'recv' handler
static void shell_recv( char* args )
{
  args = args;

#ifndef BUILD_XMODEM
  printf( "XMODEM support not compiled, unable to recv\n" );
  return;
#else // #ifndef BUILD_XMODEM

  char *p;
  long actsize;
  lua_State* L;

  if( ( shell_prog = malloc( XMODEM_INITIAL_BUFFER_SIZE ) ) == NULL )
  {
    printf( "Unable to allocate memory\n" );
    return;
  }
  printf( "Waiting for file ... " );
  if( ( actsize = xmodem_receive( &shell_prog ) ) < 0 )
  {
    free( shell_prog );
    shell_prog = NULL;
    if( actsize == XMODEM_ERROR_OUTOFMEM )
      printf( "file too big\n" );
    else
      printf( "XMODEM error\n" );
    return;
  }
  // Eliminate the XMODEM padding bytes
  p = shell_prog + actsize - 1;
  while( *p == '\x1A' )
    p --;
  p ++;
  printf( "done, got %u bytes\n", ( unsigned )( p - shell_prog ) );          
  
  // we've received an argument, try saving it to a file
  if( strcmp( args, "") != 0 )
  {
    FILE *foutput = fopen( args, "w" );
    size_t file_sz = p - shell_prog;
    if( foutput == NULL )
    {
      printf( "unable to open file %s\n", args);
      free( shell_prog );
      shell_prog = NULL;
      return;
    }
    if( fwrite( shell_prog, sizeof( char ), file_sz, foutput ) == file_sz )
      printf( "received and saved as %s\n", args );
    else
      printf( "disk full, unable to save file %s\n", args );
    fclose( foutput );
  }
  else // no arg, running the file with lua.
  {
    if( ( L = lua_open() ) == NULL )
    {
      printf( "Unable to create Lua state\n" );
      free( shell_prog );
      shell_prog = NULL;
      return;
    }
    luaL_openlibs( L );
    if( luaL_loadbuffer( L, shell_prog, p - shell_prog, "xmodem" ) != 0 )
      printf( "Error: %s\n", lua_tostring( L, -1 ) );
    else
      if( lua_pcall( L, 0, LUA_MULTRET, 0 ) != 0 )
        printf( "Error: %s\n", lua_tostring( L, -1 ) );
    lua_close( L );
  }
  free( shell_prog );
  shell_prog = NULL;
#endif // #ifndef BUILD_XMODEM
}

// ----------------------------------------------------------------------------
// 'ver' handler
static void shell_ver( char* args )
{
  args = args;
  printf( "eLua version %s\n", ELUA_STR_VERSION );
  printf( "For more information visit www.eluaproject.net and wiki.eluaproject.net\n" );
}

// ----------------------------------------------------------------------------
// 'ls' and 'dir' handler
// Iterator callback
static int shell_ls_iterator_cb( const char *fsname, struct dm_dirent *ent, void *f, int i )
{
  u32 *total = ( u32* )f;
 
  if( !ent )
  {
    if( i == -1 ) // pre-iteration
    {
      *total = 0;
      printf( TERM_FGCOL_LIGHT_BLUE "\n%s" TERM_RESET_COL, fsname );
    }
    else // post-iteration
    {
      if( i > 0 )
        printf( TERM_FGCOL_LIGHT_YELLOW "\n\nTotal on %s: %u bytes\n" TERM_RESET_COL, fsname, ( unsigned )*total );
      else // post-iteration with no files matched
        printf( TERM_FGCOL_LIGHT_MAGENTA "\n\nNo files found on %s\n" TERM_RESET_COL, fsname );
    }
    return 1;
  }
  printf( TERM_FGCOL_LIGHT_GREEN "\n  %s" TERM_RESET_COL, ent->fname );
  for( i = strlen( ent->fname ); i <= DM_MAX_FNAME_LENGTH; i++ )
    printf( " " );
  printf( TERM_FGCOL_LIGHT_CYAN "%u bytes" TERM_RESET_COL , ( unsigned )ent->fsize );
  *total = *total + ent->fsize;
  return 1;
}

static void shell_ls( char* args )
{
  const DM_DEVICE *pdev;
  unsigned dev;
  u32 total;
  char *pattern = NULL;
  char *fspattern = NULL;
  char fullpattern[ DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 2 ];

  // Check for file pattern and filesystem pattern
  if( *args != 0 )
  {
    *strchr( args, ' ' ) = 0;
    pattern = args;
    if( *pattern == '/' ) // the pattern contains a filesystem spec
    {
      fspattern = pattern;
      pattern = strchr( fspattern + 1, '/' );
      if( pattern )
      {
        pattern ++;
        if( *pattern == '\0' )
          pattern = NULL;
      }
    }
  }
  
  term_enable_paging( TERM_PAGING_ON );
  // Iterate through all devices, looking for the ones that can do "opendir"
  for( dev = 0; dev < dm_get_num_devices(); dev ++ )
  {  
    pdev = dm_get_device_at( dev );
    if( pdev->p_opendir_r == NULL || pdev->p_readdir_r == NULL || pdev->p_closedir_r == NULL )
      continue;
    // Check against the pattern
    if( fspattern && strncmp( fspattern, pdev->name, strlen( pdev->name ) ) )
      continue;
    snprintf( fullpattern, DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 1, "%s/%s", pdev->name, pattern == NULL ? "*" : pattern );
    shellh_pattern_iterator( fullpattern, shell_ls_iterator_cb, &total );
  }   
  term_enable_paging( TERM_PAGING_OFF );
  printf( "\n" );
}

// ----------------------------------------------------------------------------
// 'cat' and 'type' handler
static void shell_cat( char *args )
{
  FILE *fp;
  int c;
  char *p;

// *args has an appended space. Replace it with the string terminator.
//  *(strchr( args, ' ' )) = 0;
  if ( *args )
  {
    term_enable_paging( TERM_PAGING_ON );
    while ( *args ) 
    {
      p = strchr( args, ' ' );
      *p = 0;
      if( ( fp = fopen( args , "rb" ) ) != NULL )
      {
        c = fgetc( fp );
        while( c != EOF ) 
        {
          printf("%c", (char) c );  
          c = fgetc( fp );
        }
        fclose ( fp );
      }
      else
        printf( "File %s not found\n", args );
      args = p + 1;
    }      
    term_enable_paging( TERM_PAGING_OFF );
  }
  else
    printf( "Usage: cat (or type) <filename1> [<filename2> ...]\n" );
}

// ----------------------------------------------------------------------------
// 'edit' handler
static void shell_edit( char *args )
{
  if( *args != 0 )
    *strchr( args, ' ' ) = 0;
  editor_init( args );
  editor_mainloop();
}

// ----------------------------------------------------------------------------
// 'rm' handler
// Iterator callback
static int shell_rm_iterator_cb( const char *fsname, struct dm_dirent *ent, void *f, int i )
{
  int *pask = ( int* )f;
  char fullpattern[ DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 2 ];
  int resp;

  if( ent == NULL )
  {
    if( i == 0 ) // post-iteration with no files matched
      printf( "No files deleted.\n" );
    return 1;
  }
  snprintf( fullpattern, DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 1, "%s/%s", fsname, ent->fname );
  if( *pask )
  {
    printf( TERM_FGCOL_LIGHT_BLUE "Delete %s ", fullpattern );
    shellh_print_action_prompt();
    resp = term_getch( TERM_INPUT_WAIT );
    printf( "%c\n", ( char )resp );
    if( resp == 'x' || resp == 'X' )
      return 0;
    if( resp == 'y' || resp == 'Y' )
      if( unlink( fullpattern ) != 0 )
        printf( TERM_FGCOL_LIGHT_RED "Error deleting %s\n" TERM_RESET_COL, fullpattern );
  }
  else
  {
    printf( TERM_FGCOL_LIGHT_GREEN "Deleting %s ... ", fullpattern );
    if( unlink( fullpattern ) == 0 )
      printf( "done.\n" );
    else
      printf( TERM_FGCOL_LIGHT_RED "error!\n" );
    printf( TERM_RESET_COL );
  }
  return 1;
}

static void shell_rm( char *args )
{
  char *p1 = NULL, *p2 = NULL;
  int ptype, ask = 1;
  char *pdevname = NULL, *pattern = NULL;

  if( *args )
  {
    p1 = strchr( args, ' ' );
    if( p1 )
    {
      *p1 = 0;
      p2 = strchr( p1 + 1, ' ' );
      if( p2  )
      {
        *p2 = 0;
        if( strcasecmp( p1 + 1, "-f" ) )
        {
          printf( "Invalid argument '%s'.\n", p1 + 1 );
          return;
        }
        else
          ask = 0;
      }
      ptype = cmn_get_path_type( args, NULL );
      if( ptype == PATH_TYPE_INVALID )
        printf( "Invalid argument '%s'.\n", args );
      else if( ptype == PATH_TYPE_FS ) // force <fs>/* pattern
      {
        if( ( pdevname = shellh_pattern_from_fs( args ) ) == NULL )
          return;
        pattern = pdevname;
      }
      else
        pattern = args;
      shellh_pattern_iterator( pattern, shell_rm_iterator_cb, &ask );
    }
  }
  else
    printf( "Missing argument.\n" );
  if( pdevname )
    free( pdevname );
}

// ----------------------------------------------------------------------------
// 'ee' handler
static void shell_ee( char *args )
{
  FILE *fp;
  char eebuf[ EE_PAGE_SIZE ];
  u16 addr = 0;
  unsigned i;

  if( *args == 0 )
  {
    printf( "Must specify filename\n" );
    return;
  }
  *strchr( args, ' ' ) = 0;
  if( ( fp = fopen( args, "rb" ) ) == NULL )
  {
    printf( "Unable to open %s\n", args );
    return;
  }
  platform_i2c_setup( EE_I2C_NUM, PLATFORM_I2C_SPEED_FAST ); 
  // Copy all data to the EEPROM
  while( 1 )
  {
    memset( eebuf, 0xFF, EE_PAGE_SIZE );
    if( fread( eebuf, 1, EE_PAGE_SIZE, fp ) == 0 )
      break;
    // Start write cycle
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, DISABLE);
    platform_i2c_send_start( EE_I2C_NUM );
    platform_i2c_send_address( EE_I2C_NUM, EE_I2C_ADDR, PLATFORM_I2C_DIRECTION_TRANSMITTER );
    // Send EEPROM address
    platform_i2c_send_byte( EE_I2C_NUM, addr >> 8 );
    platform_i2c_send_byte( EE_I2C_NUM, addr & 0xFF );
    // Send data bytes
    for( i = 0; i < EE_PAGE_SIZE; i ++ )
      platform_i2c_send_byte( EE_I2C_NUM, eebuf[ i ] );
    // All done, send stop
    platform_i2c_send_stop( EE_I2C_NUM );
    // Now wait for operation complete
    while( 1 )
    {
      platform_i2c_send_start( EE_I2C_NUM );
      if( platform_i2c_send_address( EE_I2C_NUM, EE_I2C_ADDR, PLATFORM_I2C_DIRECTION_TRANSMITTER ) )
        break;
    }
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
    // Show progress and proceed with next block
    printf( "." );
    addr += EE_PAGE_SIZE; 
  }
  printf( " done, wrote %u bytes\n", ( unsigned )addr );
  fclose( fp );
}

// ----------------------------------------------------------------------------
// 'copy' handler
#ifdef BUILD_RFS
#define SHELL_COPY_BUFSIZE    ( ( 1 << RFS_BUFFER_SIZE ) - ELUARPC_WRITE_REQUEST_EXTRA )
#else
#define SHELL_COPY_BUFSIZE    256
#endif
#define SHELL_CP_FLAG_BACKUP    1
#define SHELL_CP_FLAG_CONFIRM   2
#define SHELL_CP_FLAG_OVERWRITE 4

// Copy state (for callback)
typedef struct
{
  int srctype, desttype;
  char *psrc, *pdest;
  const char *psrcname;
  int flags;
} shell_cp_state;

// Copy helper: makes the actual copy
static int shellh_cp_copy( const char *psrcname, const char *pdestname, int flags )
{
  int res = 0;
  FILE *fps = NULL, *fpd = NULL;
  void *buf = NULL;
  size_t datalen;

  // Check if the file exists first
  if( ( fps = fopen( psrcname, "rb" ) ) == NULL )
  {
    printf( TERM_FGCOL_LIGHT_RED "Unable to open %s for reading\n" TERM_RESET_COL, psrcname );
    goto cphout;
  }
  // If it exists and confirmation mode is on, ask for confirmation
  if( flags & SHELL_CP_FLAG_CONFIRM )
  {
    printf( TERM_FGCOL_LIGHT_BLUE "Copy file %s to %s ", psrcname, pdestname );
    shellh_print_action_prompt();
    printf( "%c\n", res = term_getch( TERM_INPUT_WAIT ) );
    if( toupper( res ) == 'X' ) // exit copy operation
    {
      res = -1;
      goto cphout;
    }
    else if( toupper( res ) != 'Y' ) // skip this file
    {
      res = 1;
      goto cphout;
    }
    else
      res = 0;
  }
  // If the destination exists and 'overwrite' is off, ask for confirmation
  if( ( ( flags & SHELL_CP_FLAG_OVERWRITE ) == 0 ) && ( ( fpd = fopen( pdestname, "rb" ) ) != NULL ) )
  {
    fclose( fpd );
    fpd = NULL;
    printf( TERM_FGCOL_LIGHT_BLUE "File '%s' exists, overwrite ", pdestname );
    shellh_print_action_prompt();
    printf( "%c\n", res = term_getch( TERM_INPUT_WAIT ) );
    if( toupper( res ) == 'X' ) // exit copy operation
    {
      res = -1;
      goto cphout;
    }
    else if( toupper( res ) != 'Y' ) // skip this file
    {
      res = 1;
      goto cphout;
    }
    else
      res = 0;
  }  
  if( ( fpd = fopen( pdestname, "wb" ) ) == NULL )
    printf( TERM_FGCOL_LIGHT_RED "Unable to open %s for writing\n" TERM_RESET_COL, pdestname );
  else
  {
    // Alloc memory
    if( ( buf = malloc( SHELL_COPY_BUFSIZE ) ) == NULL )
      printf( TERM_FGCOL_LIGHT_RED "Not enough memory\n" TERM_RESET_COL );
    else
    {
      // Do the actual copy
      printf( TERM_FGCOL_LIGHT_GREEN "Copying %s to %s ... ", psrcname, pdestname );
      while( 1 )
      {
        datalen = fread( buf, 1, SHELL_COPY_BUFSIZE, fps );
        if( fwrite( buf, 1, datalen, fpd ) != datalen )
        {
          printf( TERM_FGCOL_LIGHT_RED "unable to write to %s\n" TERM_RESET_COL, pdestname );
          goto cphout;
        }
        if( datalen < SHELL_COPY_BUFSIZE )
          break;
      }
      printf( TERM_FGCOL_LIGHT_BLUE "done\n" TERM_RESET_COL );
      res = 1;
    }
  }
cphout:
  if( fps )
    fclose( fps );
  if( fpd )
    fclose( fpd );
  if( buf )
    free( buf );
  return res;
}

// Copy helper: make a full name from a FS name and a file name. If 'backup' is not 0 
// prepend the file name with a "b_"
static void shellh_cp_makefname( char *pdest, const char *pfsname, const char *fname, int backup )
{
  strcpy( pdest, pfsname );
  if( pdest[ strlen( pdest ) -1 ] == '/' )
    pdest[ strlen( pdest ) - 1 ] = '\0';
  strcat( pdest, "/" );
  strcat( pdest, backup ? "b_" : "" );
  strcat( pdest, fname );
}

// Iterator callback
static int shell_cp_iterator_cb( const char *fsname, struct dm_dirent *ent, void *f, int i )
{
  shell_cp_state *pcs = ( shell_cp_state* )f;
  char fulldest[ DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 4 ];
  char fullsrc[ DM_MAX_DEV_NAME + DM_MAX_FNAME_LENGTH + 2 ];
  int res;

  if( ent == NULL )
    return 1;
  if( pcs->srctype == PATH_TYPE_FILE )
  {
    if( pcs->desttype == PATH_TYPE_FILE ) // simple copy, ignore backup flag
      res = shellh_cp_copy( pcs->psrc, pcs->pdest, pcs->flags );
    else // make dest name same as source name
    {
      shellh_cp_makefname( fulldest, pcs->pdest, pcs->psrcname, pcs->flags & SHELL_CP_FLAG_BACKUP );
      res = shellh_cp_copy( pcs->psrc, fulldest, pcs->flags );
    }
  }
  else // wildcard
  {
    snprintf( fullsrc, DM_MAX_FNAME_LENGTH + 1, "%s/%s", fsname, ent->fname );
    shellh_cp_makefname( fulldest, pcs->pdest, ent->fname, pcs->flags & SHELL_CP_FLAG_BACKUP );
    res = shellh_cp_copy( fullsrc, fulldest, pcs->flags );
  }
  return res == -1 ? 0 : 1;
}

static void shell_cp( char *args )
{
  int argno = 0;
  char *psrc = NULL, *pdest = NULL, *p;
  int flags = 0;
  int srctype, desttype;
  const char *srcname = NULL;
  char *srcwc = NULL;
  shell_cp_state cs;

  // Collect all arguments
  if ( *args )
  {
    while ( *args ) 
    {
      p = strchr( args, ' ' );
      *p = 0;
      if( argno == 0 )
        psrc = args;
      else if( argno == 1 )
        pdest = args;
      else
      {
        if( !strcasecmp( args, "-b" ) )
          flags |= SHELL_CP_FLAG_BACKUP;
        else if( !strcasecmp( args, "-c" ) )
          flags |= SHELL_CP_FLAG_CONFIRM;
        else if( !strcasecmp( args, "-o" ) )
          flags |= SHELL_CP_FLAG_OVERWRITE;
        else
        {
          printf( TERM_FGCOL_LIGHT_RED "Invalid argument '%s'\n" TERM_RESET_COL, args );
          return;
        }
      }
      args = p + 1;
      argno ++;
    }      
  }
  if( !psrc || !pdest )
  {
    printf( TERM_FGCOL_LIGHT_RED "Must specify both source and destination\n" TERM_RESET_COL );
    return;
  }
  srctype = cmn_get_path_type( psrc, &srcname );
  desttype = cmn_get_path_type( pdest, NULL );
  // Check source
  if( srctype == PATH_TYPE_INVALID )
  {
    printf( TERM_FGCOL_LIGHT_RED "Invalid source.\n" TERM_RESET_COL );
    return;
  }
  else if( srctype == PATH_TYPE_FS ) // transform this into a pattern
  {
    if( ( psrc = srcwc = shellh_pattern_from_fs( psrc ) ) == NULL )
      return;
    srctype = cmn_get_path_type( psrc, &srcname );
  }
  // Check destination
  if( desttype == PATH_TYPE_INVALID || desttype == PATH_TYPE_WC )
  {
    printf( TERM_FGCOL_LIGHT_RED "Invalid destination.\n" TERM_RESET_COL );
    goto cpdone;
  }
  if( srctype == PATH_TYPE_WC && desttype == PATH_TYPE_FILE )
  {
    printf( TERM_FGCOL_LIGHT_RED "Invalid source/destination combination.\n" TERM_RESET_COL );
    goto cpdone;
  }
  // Check for invalid backup flag
  if( desttype == PATH_TYPE_FILE && ( ( flags & SHELL_CP_FLAG_BACKUP ) != 0 ) )
  {
    printf( TERM_FGCOL_LIGHT_RED "Cannot use the backup flag with a specific destination.\n" TERM_RESET_COL );
    goto cpdone;
  }
  // Prepares state for iteration
  cs.srctype = srctype;
  cs.desttype = desttype;
  cs.psrc = psrc;
  cs.pdest = pdest;
  cs.psrcname = srcname;
  cs.flags = flags;
  shellh_pattern_iterator( psrc, shell_cp_iterator_cb, &cs );
cpdone:
  if( srcwc )
    free( srcwc );
}

// ----------------------------------------------------------------------------
// Shell command table
// Insert shell commands here
static const SHELL_COMMAND shell_commands[] =
{
  { "help", shell_help },
  { "lua", shell_lua },
  { "recv", shell_recv },
  { "ver", shell_ver },
  { "exit", NULL },
  { "ls", shell_ls },
  { "dir", shell_ls },
  { "cat", shell_cat },
  { "type", shell_cat },
  { "cp", shell_cp },
  { "ee", shell_ee },
  { "edit", shell_edit },
  { "rm", shell_rm },
  { NULL, NULL }
};

// Execute the eLua "shell" in an infinite loop
void shell_start()
{
  char cmd[ SHELL_MAXSIZE + 1 ];
  char *p, *temp;
  const SHELL_COMMAND* pcmd;
  int i, inside_quotes;
  char quote_char;

  printf( SHELL_WELCOMEMSG, ELUA_STR_VERSION );
  while( 1 )
  {
    while( linenoise_getline( LINENOISE_ID_SHELL, cmd, SHELL_MAXSIZE, SHELL_PROMPT ) == -1 )
    {
      printf( "\n" );
      clearerr( stdin );
    }
    if( strlen( cmd ) == 0 )
      continue;
    linenoise_addhistory( LINENOISE_ID_SHELL, cmd );
    if( cmd[ strlen( cmd ) - 1 ] != '\n' )
      strcat( cmd, "\n" );

    // Change '\r' and '\n' chars to ' ' to ease processing
    p = cmd;
    while( *p )
    {
      if( *p == '\r' || *p == '\n' )
        *p = ' ';
      p ++;
    }

    // Transform ' ' characters inside a '' or "" quoted string in
    // a 'special' char. We do this to let the user execute something
    // like "lua -e 'quoted string'" without disturbing the quoted
    // string in any way.
    for( i = 0, inside_quotes = 0, quote_char = '\0'; i < strlen( cmd ); i ++ )
      if( ( cmd[ i ] == '\'' ) || ( cmd[ i ] == '"' ) )
      {
        if( !inside_quotes )
        {
          inside_quotes = 1;
          quote_char = cmd[ i ];
        }
        else
        {
          if( cmd[ i ] == quote_char )
          {
            inside_quotes = 0;
            quote_char = '\0';
          }
        }
      }
      else if( ( cmd[ i ] == ' ' ) && inside_quotes )
        cmd[ i ] = SHELL_ALT_SPACE;
    if( inside_quotes )
    {
      printf( "Invalid quoted string\n" );
      continue;
    }

    // Transform consecutive sequences of spaces into a single space
    p = strchr( cmd, ' ' );
    while( p )
    {
      temp = p + 1;
      while( *temp && *temp == ' ' )
        memmove( temp, temp + 1, strlen( temp ) );
      p = strchr( p + 1, ' ' );
    }
    if( strlen( cmd ) == 1 )
      continue;

    // Look for the first ' ' to separate the command from its args
    temp = cmd;
    if( *temp == ' ' )
      temp ++;
    if( ( p = strchr( temp, ' ' ) ) == NULL )
    {
      printf( SHELL_ERRMSG );
      continue;
    }
    *p = 0;
    i = 0;
    while( 1 )
    {
      pcmd = shell_commands + i;
      if( pcmd->cmd == NULL )
      {
        printf( SHELL_ERRMSG );
        break;
      }
      if( !strcasecmp( pcmd->cmd, temp ) )
      {
        // Special case: the "exit" command has a NULL handler
        if( pcmd->handler_func )
          pcmd->handler_func( p + 1 );
        break;
      }
      i ++;
    }
    // Check for 'exit' command
    if( pcmd->cmd && !pcmd->handler_func )
#ifdef BUILD_UIP
    {
      if( ( i = elua_net_get_telnet_socket() ) != -1 )
        elua_net_close( i );
    }
#else
      break;
#endif

  }
  // Shell exit point
  if( shell_prog )
    free( shell_prog );
}

// Initialize the shell, returning 1 for OK and 0 for error
int shell_init()
{
  shell_prog = NULL;
  return 1;
}

#else // #ifdef BUILD_SHELL

int shell_init()
{
  return 0;
}

void shell_start()
{
}

#endif // #ifdef BUILD_SHELL
