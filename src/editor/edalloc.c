// Editor allocator

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"
#include "edalloc.h"
#include "type.h"
#include "edutils.h"

// *****************************************************************************
// Local data

#define ZONE_FREE             0
#define ZONE_USED             1

// *****************************************************************************
// Local functions

static int edalloc_extra_space;

// -----------------------------------------------------------------------------

// Round up a memory size to a multiple of LINE_ALLOCATOR_ZONE_SIZE
static unsigned edalloc_round_size( unsigned size )
{
  return ( ( size + LINE_ALLOCATOR_ZONE_SIZE - 1 ) / LINE_ALLOCATOR_ZONE_SIZE ) * LINE_ALLOCATOR_ZONE_SIZE;
}

// *****************************************************************************
// Public interface

void* edalloc_line_malloc( unsigned size )
{
  // Get size in zones
  //return malloc( size );
  if( ( size = edalloc_round_size( size ) ) == 0 )
    return NULL;
  return malloc( size );
}

void edalloc_line_free( void* ptr )
{
  free( ptr );
}

void* edalloc_line_realloc( void* ptr, unsigned size )
{
  char *s = ( char* )ptr;

  if( edalloc_round_size( size ) == edalloc_round_size( strlen( s ) + 1 ) )
    return ptr;
  return realloc( ptr, edalloc_round_size( size ) );
}

// Set the free space that will be skipped after a line
// Used in the initial phase when the lines are read to allow for some basic
// line editing without having to move memory around
void edalloc_line_set_extra_space( int size )
{
  edalloc_extra_space = edalloc_round_size( size ); 
}

// Free memory from an editor buffer
void edalloc_free_buffer( EDITOR_BUFFER *b )
{
  unsigned i;

  if( b )
  {
    if( b->fpath )
      free( b->fpath );
    if( b->lines )
    {
      for( i = 0; i < b->file_lines; i ++ )
        if( b->lines[ i ] )
          edalloc_line_free( b->lines[ i ] );
      free( b->lines );
    }
    free( b );
  }
}

// Create a new buffer from a file name reading all the file data
EDITOR_BUFFER* edalloc_buffer_new( const char *fname )
{
  FILE *fp = NULL;
  EDITOR_BUFFER *b = NULL;
  s32 fsize = 0;
  unsigned i;
  char *linebuf = NULL;
  char *s;

  // Allocate the temporary line buffer 
  if( ( linebuf = ( char* )malloc( LINE_BUFFER_SIZE + 1 ) ) == NULL )
    goto newout;

  if( fname && *fname )
  {
    if( ( fp = fopen( fname, "rb" ) ) == NULL )
      goto newout;

    // Get file size
    if( fseek( fp, 0, SEEK_END ) == -1 )
      goto newout;
    fsize = ftell( fp );
    fseek( fp, 0, SEEK_SET );
  }
  else
    fname = NULL;

  // Allocate the buffer now
  if( ( b = ( EDITOR_BUFFER* )malloc( sizeof( EDITOR_BUFFER ) ) ) == NULL )
    goto newout;
  memset( b, 0, sizeof( EDITOR_BUFFER ) );
  if( fname )
  {
    if( ( b->fpath = strdup( fname ) ) == NULL )
      goto newout;
  }

  // Estimate the initial number of lines and allocate them
  b->allocated_lines = fsize / FILE_ESTIMATED_LINE_SIZE + BUFFER_ALLOCATOR_EXTRA_LINES;
  if( ( b->lines = ( char** )malloc( sizeof( char* ) * b->allocated_lines ) ) == NULL )
    goto newout;
  memset( b->lines, 0, sizeof( char* ) * b->allocated_lines );

  // Read the file line by line
  i = 0;
  if( fp )
  {
    while( 1 )
    {
      if( fgets( linebuf, LINE_BUFFER_SIZE, fp ) == NULL )
        break;
      // Strip '\r' and '\n' from the end of the string
      s = linebuf + strlen( linebuf ) - 1;
      while( s >= linebuf && ( *s == '\r' || *s == '\n' ) )
        s --;
      *( s + 1 ) = '\0';
      // Use the specialized line allocator to alloc a line
      if( ( b->lines[ i ] = edalloc_line_malloc( strlen( linebuf ) + 1 ) ) == NULL )
        goto newout;
      strcpy( b->lines[ i ++ ], linebuf );
      if( !edalloc_buffer_change_lines( b, 1 ) )
        goto newout;
    }
    fclose( fp );
  }
  else
  {
    // Empty buffer: create a file with a single initial line and mark it as initially empty
    if( ( b->lines[ 0 ] = edalloc_line_malloc( LINE_ALLOCATOR_ZONE_SIZE ) ) == NULL )
      goto newout;
    *b->lines[ 0 ] = '\0';
    if( !edalloc_buffer_change_lines( b, 1 ) )
      goto newout;
    edutils_set_flag( b, EDFLAG_WAS_EMPTY, 1 );
  }
  // Everything is OK, return the new buffer
  free( linebuf );
  return b;

newout:  
  if( fp )
    fclose( fp );
  edalloc_free_buffer( b );
  if( linebuf )
    free( linebuf );
  return NULL;
}

// Set the filename for a buffer
int edalloc_set_fname( EDITOR_BUFFER *b, const char *name )
{
  if( name && *name )
  {
    if( b->fpath )
      free( b->fpath );
    if( ( b->fpath = strdup( name ) ) == NULL )
      return 0;
  }
  return 1;
}

// Change the number of lines in the given buffer
int edalloc_buffer_change_lines( EDITOR_BUFFER* b, int delta )
{
  int must_change = 0;

  b->file_lines += delta;
  if( delta > 0 )
  {
    if( b->file_lines >= b->allocated_lines ) // Need to allocate more lines
      must_change = 1;
  }
  else
  {
    if( b->file_lines + 2 * BUFFER_ALLOCATOR_EXTRA_LINES < b->allocated_lines ) // remove some lines
      must_change = 1;
  }
  if( must_change )
  {
    b->allocated_lines = b->file_lines + BUFFER_ALLOCATOR_EXTRA_LINES;
    if( ( b->lines = ( char** )realloc( b->lines, sizeof( char* ) * b->allocated_lines ) ) == NULL )
      return 0;
  }
  return 1;
}

// Initialize the allocator
// Returns 1 for OK, 0 for error
int edalloc_init()
{
  return 1;
}

// Reclaim all memory requested by the allocator
void edalloc_deinit()
{
}

void edalloc_buffer_remove_line( EDITOR_BUFFER* b, int line )
{
  int total = b->file_lines;

  edalloc_line_free( b->lines[ line ] );
  if( line < total - 1 )
    memmove( b->lines + line, b->lines + line + 1, ( total - line - 1 ) * sizeof( char* ) );
  edalloc_buffer_change_lines( b, -1 );
}

int edalloc_buffer_add_line( EDITOR_BUFFER* b, int line, char* pline )
{
  int total;

  if( edalloc_buffer_change_lines( b, 1 ) == 0 )
    return 0;
  total = b->file_lines;
  if( line < total - 1 )
    memmove( b->lines + line + 1, b->lines + line, ( total - line - 1 ) * sizeof( char* ) );
  b->lines[ line ] = pline;
  return 1;
}

// Clear the selection buffer
void edalloc_clear_selection( EDITOR_BUFFER *b )
{
  int i;

  if( b->sellines == NULL )
    return;
  for( i = b->firstsel; i <= b->lastsel; i ++ )
    if( b->sellines[ i - b->firstsel ] )
      edalloc_line_free( b->sellines[ i - b->firstsel ] );
  free( b->sellines );
  b->sellines = NULL;
  b->firstsel = b->lastsel = -1;
}

// Reset an used selection by clearing only the main array, not the lines
// (which are now part of the editor).
void edalloc_reset_used_selection( EDITOR_BUFFER *b )
{
  if( b->sellines == NULL )
    return;
  free( b->sellines );
  b->sellines = NULL;
  b->firstsel = b->lastsel = -1;
}

// Fill the selection buffer
int edalloc_fill_selection( EDITOR_BUFFER *b )
{
  int i, total = b->lastsel - b->firstsel + 1;

  if( ( b->sellines = ( char ** )malloc( total * sizeof( char* ) ) ) == NULL )
    return 0;
  memset( b->sellines, 0, total * sizeof( char* ) );
  for( i = 0; i < total; i ++ )
  {
    if( ( b->sellines[ i ] = edalloc_line_malloc( strlen( edutils_line_get( i + b->firstsel ) ) + 1 ) ) == NULL )
      goto error;
    strcpy( b->sellines[ i ], edutils_line_get( i + b->firstsel ) );
  }
  return 1;
error:
  edalloc_clear_selection( b );
  return 0;
}


