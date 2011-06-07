// Editor allocator

#ifndef __EDALLOC_H__
#define __EDALLOC_H__

#include "type.h"
#include "editor.h"

// Static editor line configuration
// The next constant used to be 20 for the zones allocator
#define LINE_ALLOCATOR_ZONE_SIZE        24        // the "zone" is the minimal (and fixed) unit of allocatoin
#define LINE_ALLOCATOR_ZONES            500       // size of allocator in zones
#define BUFFER_ALLOCATOR_EXTRA_LINES    10        // how many more lines to allocate on each request
#define FILE_ESTIMATED_LINE_SIZE        40        // estimated medium size (in chars) of an editor line
#define LINE_BUFFER_SIZE                500       // size of the line buffer (gives the maximum length of a line in the file)

// Editor line allocator
int edalloc_init();
void edalloc_deinit();
EDITOR_BUFFER* edalloc_buffer_new( const char *fname );
void edalloc_free_buffer( EDITOR_BUFFER *b );
int edalloc_buffer_change_lines( EDITOR_BUFFER* b, int delta );
void* edalloc_line_malloc( unsigned size );
void edalloc_line_free( void* ptr );
void* edalloc_line_realloc( void* ptr, unsigned size );
void edalloc_line_set_extra_space( int size );
void edalloc_buffer_remove_line( EDITOR_BUFFER* b, int line );
int edalloc_buffer_add_line( EDITOR_BUFFER* b, int line, char* pline );
int edalloc_set_fname( EDITOR_BUFFER *b, const char *name );
void edalloc_clear_selection( EDITOR_BUFFER *b );
void edalloc_reset_used_selection( EDITOR_BUFFER *b );
int edalloc_fill_selection( EDITOR_BUFFER *b );

#endif

