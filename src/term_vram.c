// Terminal function

#include "term.h"
#include "type.h"
#include "platform.h"
#include "vram.h"
#include <stdio.h>

#include "platform_conf.h"
#ifdef BUILD_TERM_VRAM

// Local variables
static p_term_out term_out;
static p_term_in term_in;
static p_term_translate term_translate;

// *****************************************************************************
// Terminal functions

// Clear the screen
void term_clrscr()
{
  vram_clrscr();
}

// Clear to end of line
void term_clreol()
{
  vram_clreol();
}

// Move cursor to (x, y)
void term_gotoxy( unsigned x, unsigned y )
{
  vram_gotoxy( x, y );
}

// Move cursor up "delta" lines
void term_up( unsigned delta )
{
  vram_deltaxy( 0, ( int )-delta );
}

// Move cursor down "delta" lines
void term_down( unsigned delta )
{
  vram_deltaxy( 0, -delta );
}

// Move cursor right "delta" chars
void term_right( unsigned delta )
{
  vram_deltaxy( delta, 0 );
}

// Move cursor left "delta" chars
void term_left( unsigned delta )
{
  vram_deltaxy( ( int )-delta, 0 );
}

// Return the number of terminal lines
unsigned term_get_lines()
{
  return VRAM_LINES;
}

// Return the number of terminal columns
unsigned term_get_cols()
{
  return VRAM_COLS;
}

// Write a character to the terminal
void term_putch( u8 ch )
{
  vram_putchar( ch );
}

// Write a string to the terminal
void term_putstr( const char* str, unsigned size )
{
  while( size )
  {
    vram_putchar( *str );
    str ++;
    size --;
  }
}
 
// Return the cursor "x" position
unsigned term_get_cx()
{
  return vram_get_cx();
}

// Return the cursor "y" position
unsigned term_get_cy()
{
  return vram_get_cy();
}

void term_set_color( int fgcol, int bgcol )
{
  vram_set_color( fgcol, bgcol );
}

// Return a char read from the terminal
// If "mode" is TERM_INPUT_DONT_WAIT, return the char only if it is available,
// otherwise return -1
// Calls the translate function to translate the terminal's physical key codes
// to logical key codes (defined in the term.h header)
int term_getch( int mode )
{
  int ch;
  
  if( ( ch = term_in( mode ) ) == -1 )
    return -1;
  else
    return term_translate( ch );
}

void term_init( unsigned lines, unsigned cols, p_term_out term_out_func, 
                p_term_in term_in_func, p_term_translate term_translate_func )
{
  lines = lines;
  cols = cols;
  term_out = term_out_func;
  term_in = term_in_func;
  term_translate = term_translate_func;
}                

#else // #ifdef BUILD_TERM

void term_init( unsigned lines, unsigned cols, p_term_out term_out_func, 
                p_term_in term_in_func, p_term_translate term_translate_func )
{
}

#endif // #ifdef BUILD_TERM
