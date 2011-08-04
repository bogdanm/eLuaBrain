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

// ****************************************************************************
// Shell functions

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
  printf( "  cp <src> <dst> - copy source file 'src' to 'dst'\n" );
  printf( "  ee <file>   - dump file to the EEPROM connected on I2C1\n" );
  printf( "  edit <file> - edits the given file\n" );
  printf( "  ver         - print eLua version\n" );
  printf( "  rm <file> <-f> - removes the file, use '-f' to supress confirmation\n" );
}

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

// 'ver' handler
static void shell_ver( char* args )
{
  args = args;
  printf( "eLua version %s\n", ELUA_STR_VERSION );
  printf( "For more information visit www.eluaproject.net and wiki.eluaproject.net\n" );
}

// 'ls' and 'dir' handler
static void shell_ls( char* args )
{
  const DM_DEVICE *pdev;
  unsigned dev, i;
  DM_DIR *d;
  struct dm_dirent *ent;
  u32 total;
  char *pattern = NULL;
  char *fspattern = NULL;

  // Check for pattern
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
        if( strlen( pattern ) == 0 )
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
    d = dm_opendir( pdev->name );
    if( d )
    {
      total = 0;
      printf( "\n%s", pdev->name );
      while( ( ent = dm_readdir( d ) ) != NULL )
      {
        if( cmn_match_fname( ent->fname, pattern ) )
        {
          printf( "\n%s", ent->fname );
          for( i = strlen( ent->fname ); i <= DM_MAX_FNAME_LENGTH; i++ )
            printf( " " );
          printf( "%u bytes", ( unsigned )ent->fsize );
          total = total + ent->fsize;
        }
      }
      printf( "\n\nTotal on %s: %u bytes\n", pdev->name, ( unsigned )total );
      dm_closedir( d );
    }
  }   
  term_enable_paging( TERM_PAGING_OFF );
  printf( "\n" );
}

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

// 'edit' handler
static void shell_edit( char *args )
{
  if( *args != 0 )
    *strchr( args, ' ' ) = 0;
  editor_init( args );
  editor_mainloop();
}

// 'rm' handler
static void shell_rm( char *args )
{
  char *p1 = NULL, *p2 = NULL;
  int res = 0;
  int ask = 1;

  if( *args )
  {
    p1 = strchr( args, ' ' );
    if( p1 )
    {
      *p1 = 0;
      p2 = strchr( p1 + 1, ' ' );
      if( p2  )
      {
        if( strcasecmp( p2, "-f" ) )
        {
          printf( "Invalid argument '%s'\n", p2 );
          goto rmdone;
        }
        else
          ask = 0;
      }
      // [TODO] make 'ask' count only if multiple files are removed
      res = unlink( p1 ) == 0 ? 1 : 0; 
    }
  }
rmdone:  
  if( !res )
    printf( "Remove failed.\n" );
}

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

// 'copy' handler
#ifdef BUILD_RFS
#define SHELL_COPY_BUFSIZE    ( ( 1 << RFS_BUFFER_SIZE ) - ELUARPC_WRITE_REQUEST_EXTRA )
#else
#define SHELL_COPY_BUFSIZE    256
#endif
static void shell_cp( char *args )
{
  char *p1, *p2;
  int res = 0;
  FILE *fps = NULL, *fpd = NULL;
  void *buf = NULL;
  size_t datalen, total = 0;

  if( *args )
  {
    p1 = strchr( args, ' ' );
    if( p1 )
    {
      *p1 = 0;
      p2 = strchr( p1 + 1, ' ' );
      if( p2 )
      {
        *p2 = 0;
        // First file is at args, second one at p1 + 1
        if( ( fps = fopen( args, "rb" ) ) == NULL )
          printf( "Unable to open %s for reading\n", args );
        else
        {
          if( ( fpd = fopen( p1 + 1, "wb" ) ) == NULL )
            printf( "Unable to open %s for writing\n", p1 + 1 );
          else
          {
            // Alloc memory
            if( ( buf = malloc( SHELL_COPY_BUFSIZE ) ) == NULL )
              printf( "Not enough memory\n" );
            else
            {
              // Do the actual copy
              while( 1 )
              {
                datalen = fread( buf, 1, SHELL_COPY_BUFSIZE, fps );
                if( fwrite( buf, 1, datalen, fpd ) != datalen )
                {
                  printf( "Unable to write to %s\n", p1 + 1 );
                  goto out;
                }
                total += datalen;
                if( datalen < SHELL_COPY_BUFSIZE )
                  break;
              }
              printf( "%u bytes copied\n", ( unsigned int )total );
              res = 1;
            }
          }
        } 
      }
    }
  }
out:  
  if( !res )
    printf( "Copy error.\n" );
  if( fps )
    fclose( fps );
  if( fpd )
    fclose( fpd );
  if( buf )
    free( buf );
}

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
