// Online help reader

#include "platform_conf.h"

#ifdef BUILD_HELP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "help.h"
#include "devman.h"

// ****************************************************************************
// Local variables and macros

static FILE *help_fp;
static const char* help_file_address; 
static int help_num_modules = -1;
static char *help_mod_table;
static unsigned help_mod_table_size;
static char *help_crt_module;
static HELP_MOD_DATA *help_crt_module_data;
static int help_crt_module_nfuncs = -1;

#define HELP_SIG              "eHLP"
#define HELP_SIG_SIZE         4
#define HELP_MOD_ENTRY_DSIZE  6
#define HELP_MOD_EXTRA_OFFSET 8
#define HELP_MODNAME_ALIGN    12

// This defines an entry in the module table
typedef struct
{
  const char *name;
  const char *overview;
  u32 offset;
  u16 len;
} HELP_MODTABLE_DATA;


// ****************************************************************************
// Helpers

// Cleans up a previously loaded module
static void helph_unload_module()
{
  if( help_crt_module )
  {
    if( !help_file_address )
      free( help_crt_module );
    help_crt_module = NULL;
    help_crt_module_nfuncs = -1;
  }
  if( help_crt_module_data )
  {
    free( help_crt_module_data->pfuncs );
    free( help_crt_module_data );
    help_crt_module_data = NULL;
  }
}

// Cleans up a help session
static void helph_cleanup()
{
  if( help_fp != NULL )
  {
    fclose( help_fp );
    help_fp = NULL;
  }
  if( help_mod_table )
  {
    if( !help_file_address )
      free( help_mod_table );
    help_mod_table_size = 0;
    help_mod_table = NULL;
  }
  helph_unload_module();
  help_file_address = NULL;
  help_num_modules = -1;
}

// Returns the entry at the given index in the module table
static HELP_MODTABLE_DATA* helph_get_modtable_data( int index )
{
  int i = 0;
  char *p = help_mod_table;
  static HELP_MODTABLE_DATA md;

  if( help_mod_table == NULL || index < 0 || index >= help_num_modules )
    return NULL;
  while( i < index )
  {
    p += strlen( p ) + 1;
    p += strlen( p ) + 1;
    p += HELP_MOD_ENTRY_DSIZE;
    i ++;
  }
  md.name = p;
  p += strlen( p ) + 1;
  md.overview = p;
  p += strlen( p ) + 1;
  md.offset = *( u32* )p;
  p += 4;
  md.len = *( u16* )p;
  return &md;
}

// Loads a module into memory
// Returns a pointer to the module data if OK, NULL for error
static HELP_MOD_DATA* helph_load_module( int idx )
{
  HELP_MODTABLE_DATA *pm;
  char *p;
  u16 temp16;
  int i;

  if( help_fp == NULL || help_mod_table == NULL || idx < 0 || idx >= help_num_modules )
    return NULL;
  if( ( pm = helph_get_modtable_data( idx ) ) == NULL )
    return NULL;
  helph_unload_module();
  if( !help_file_address )
  {
    if( ( help_crt_module = ( char* )malloc( pm->len ) ) == NULL )
      return NULL;
    if( fseek( help_fp, pm->offset + help_mod_table_size + HELP_MOD_EXTRA_OFFSET, SEEK_SET ) != 0 )
      goto error;
    if( fread( help_crt_module, 1, pm->len, help_fp ) != pm->len )
      goto error;
  }
  else
    help_crt_module = ( char* )help_file_address + pm->offset + help_mod_table_size + HELP_MOD_EXTRA_OFFSET;
  // Allocate the module data structure
  if( ( help_crt_module_data = ( HELP_MOD_DATA* )malloc( sizeof( HELP_MOD_DATA ) ) ) == NULL )
    goto error;
  // Compute number of functions
  help_crt_module_nfuncs = 0;
  p = help_crt_module;
  help_crt_module_data->name = p;
  p += strlen( p ) + 1; // skip over name
  help_crt_module_data->desc = p;
  p += strlen( p ) + 1; // skip over description
  // Read number of functions
  help_crt_module_nfuncs = *( u16* )p;
  p += 2;
  help_crt_module_data->nfuncs = help_crt_module_nfuncs;
  if( ( help_crt_module_data->pfuncs = ( const char ** )malloc( sizeof( char* ) * help_crt_module_nfuncs ) ) == NULL )
    goto error;
  for( i = 0; i < help_crt_module_nfuncs; i ++ )
  {
    p += strlen( p ) + 1;
    temp16 = *( u16* )p;
    help_crt_module_data->pfuncs[ i ] = p + 2;
    p += temp16 + 2;
  }
  return help_crt_module_data;
error:
  helph_unload_module();
  return NULL;
}

// Find a module in the module table by name, returning its index or -1 on error
static int helph_module_name_to_idx( const char *pname )
{
  int i;
  HELP_MODTABLE_DATA *pm;

  if( help_fp == NULL )
    return -1;
  for( i = 0; i < help_num_modules; i ++ )
  {
    pm = helph_get_modtable_data( i );
    if( strcasecmp( pm->name, pname ) == 0 )
      return i;
  }
  return -1;
}

// Find a function in a module data by name, return a pointer to its data or
// NULL on error

static const char* helph_find_function_in_module( const HELP_MOD_DATA *pm, const char* pname )
{
  int i;

  for( i = 0; i < pm->nfuncs; i ++ )
    if( strcasecmp( pm->pfuncs[ i ], pname ) == 0 )
      return pm->pfuncs[ i ];
  return NULL;
}

// ****************************************************************************
// Public interface

// Initialize help system with the given file name
// Returns 1 if OK, 0 for error
int help_init( const char *fname )
{
  char s[ HELP_SIG_SIZE + 1 ];
  u32 temp32;
  char *p;

  helph_cleanup();
  if( ( help_fp = fopen( fname, "rb" ) ) == NULL )
    return 0;
  help_file_address = dm_getaddr( fileno( help_fp ) );
  if( fread( s, 1, HELP_SIG_SIZE, help_fp ) != HELP_SIG_SIZE )
    goto error;
  s[ HELP_SIG_SIZE ] = 0;
  if( strcmp( s, HELP_SIG ) )
    goto error;
  // Help file identified, now read its header (module information)
  if( fread( &temp32, 1, 4, help_fp ) != 4 )
    goto error;
  if( !help_file_address )
  {
    if( ( help_mod_table = ( char* )malloc( temp32 ) ) == NULL )
      goto error;
    if( fread( help_mod_table, 1, temp32, help_fp ) != temp32 )
      goto error;
  }
  else
    help_mod_table = ( char* )help_file_address + ftell( help_fp );
  p = help_mod_table;
  help_mod_table_size = ( unsigned )temp32;
  help_num_modules = 0;
  while( p < help_mod_table + temp32 )
  {
    help_num_modules ++;
    p += strlen( p ) + 1;
    p += strlen( p ) + 1;
    p += HELP_MOD_ENTRY_DSIZE; // skip name, overview, offset, entry size
  }
  return 1;
error:
  helph_cleanup();
  return 0;
}

void help_close()
{
  helph_cleanup();
}

int help_get_num_modules()
{
  return help_num_modules;
}

const char* help_get_module_name_at( int index )
{
  HELP_MODTABLE_DATA *pm = helph_get_modtable_data( index );

  return pm ? pm->name : NULL;
}

const char* help_get_module_overview_at( int index )
{
  HELP_MODTABLE_DATA *pm = helph_get_modtable_data( index );

  return pm ? pm->overview : NULL;
}

HELP_MOD_DATA* help_load_module_idx( int index )
{
  return helph_load_module( index );
}

HELP_MOD_DATA* help_load_module_name( const char *name )
{
  int index = helph_module_name_to_idx( name );

  return index == -1 ? NULL : helph_load_module( index );
}

void help_unload_module()
{
  helph_unload_module();
}

// Main interface: prints the help on a given topic
void help_help( const char* topic )
{
  unsigned i, j;
  HELP_MOD_DATA *pm;
  const char *pdot;
  char *modname;
  const char *pfunc;

  if( help_fp == NULL )
    return;
  if( !topic || *topic == '\0' ) // no module/function, list all modules
  {
    printf( HLBLUE "Available modules:\n" HRESET );
    for( i = 0; i < help_num_modules; i ++ )
    {
      pdot = help_get_module_overview_at( i );
      printf( HLGREEN "  %s" HRESET, help_get_module_name_at( i ) );
      for( j = strlen( help_get_module_name_at( i ) ); j < HELP_MODNAME_ALIGN; j ++ )
        printf( " " );
      printf( " - " );
      if( *pdot == 0 )
        printf( HLRED "(overview not available)\n" HRESET );
      else
        printf( HLYELLOW "%s\n" HRESET, pdot );
   }
  }
  else if( *topic == '*' ) // *module: list all functions with their description
  {
    topic ++;
    if( ( pm = help_load_module_name( topic ) ) == NULL )
    {
      printf( HLRED "Module '%s' not found\n" HRESET, topic );
      return;
    }
    printf( HLBLUE "Module: " HRESET "%s\n%s\n", pm->name, pm->desc );
    for( i = 0; i < pm->nfuncs; i ++ )
      printf( "%s\n", pm->pfuncs[ i ] + strlen( pm->pfuncs[ i ] ) + 1 );
    help_unload_module();
  }
  else if( helph_module_name_to_idx( topic ) != -1 ) // is this a module?
  {
    if( ( pm = help_load_module_name( topic ) ) == NULL )
    {
      printf( HLRED "Module '%s' not found\n" HRESET, topic );
      return;
    }
    printf( HLBLUE "Module: " HRESET "%s\n%s\n", pm->name, pm->desc );
    printf( HLBLUE "Function list: \n" HRESET );
    for( i = j = 0; i < pm->nfuncs; i ++ )
      printf( HLGREEN "  %s\n" HRESET, pm->pfuncs[ i ] );
    help_unload_module();
  }
  else // this must be a function
  {
    if( ( pdot = strchr( topic, '.' ) ) == NULL )
    {
      printf( HLRED "Module '%s' not found\n" HRESET, topic );
      return;
    }
    // Basic sanity check
    if( pdot == topic || pdot == topic + strlen( topic ) - 1 )
    {
      printf( HLRED "Invalid help syntax\n" HRESET );
      return;
    }
    // Now loop after all '.' chars until we find the right module/function combo
    pdot = topic;
    while( *pdot )
    {
      if( ( pdot = strchr( pdot, '.' ) ) == NULL )
        break;
      if( ( modname = strndup( topic, pdot - topic ) ) == NULL )
      {
        printf( HLRED "Not enough memory\n" HRESET );
        return;
      }
      pdot ++;
      if( ( pm = help_load_module_name( modname ) ) == NULL )
      {
        free( modname );
        continue;
      }
      if( ( pfunc = helph_find_function_in_module( pm, topic ) ) == NULL )
      {
        free( modname );
        help_unload_module();
        continue;
      }
      pfunc += strlen( pfunc ) + 1;
      printf( "%s\n", pfunc );
      free( modname );
      help_unload_module();
      return;
    }
    printf( HLRED "Unable to find module or function '%s'\n" HRESET, topic );
  }
}

#endif // #ifdef BUILD_HELP

