// Editor data and public interface

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "type.h"
#ifdef EDITOR_STANDALONE
#include <ncurses.h>
#define PRINTF        printw
#else
#define PRINTF        printf
#include "platform_conf.h"
#endif


#define EMIN( x, y )                  ( ( x ) < ( y ) ? ( x ) : ( y ) )
#define EMAX( x, y )                  ( ( x ) > ( y ) ? ( x ) : ( y ) )
#define EDITOR_LINES                  ( TERM_LINES - 1 )

// Editor buffer (holds a file)
typedef struct 
{
  u8 flags;                             // buffer flags
  char *fpath;                          // complete file name
  int file_lines;                       // number of lines in file
  int allocated_lines;                  // number of lines in the "lines" array
  char** lines;                         // pointers to each line in file (NULL for lines that don't exist yet).
} EDITOR_BUFFER;

// Buffer flags
#define EDFLAG_DIRTY                  1 // is the buffer dirty ?
#define EDFLAG_WAS_EMPTY              2 // was the buffer initially empty when loaded ?

// Cursor types
enum
{
  EDITOR_CURSOR_OFF,
  EDITOR_CURSOR_UNDERLINE,
  EDITOR_CURSOR_BLOCK
}; 

int editor_init( const char* fname );
int editor_mainloop();

#endif

