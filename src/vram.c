// Direct video RAM interfacing module

#include "vram.h"
#include "type.h"
#include "string.h"
#include "term.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// *****************************************************************************
// Local variables

u32 vram_data[ VRAM_SIZE_TOTAL >> 2 ];
enum
{
  VRAM_OFF_CY,
  VRAM_OFF_CX,
  VRAM_DUMMY,
  VRAM_OFF_TYPE,
  VRAM_FIRST_DATA
};
static u8 *vram_p_cx, *vram_p_cy, *vram_p_type;
static u8 vram_fg_col = VRAM_DEFAULT_FG_COL;
static u8 vram_bg_col = VRAM_DEFAULT_BG_COL;
static u8 vram_paging_enabled;
static u8 vram_paging_lines;
static u8 vram_last_line;
static u8 vram_crt_attr;

#define VRAM_CHARADDR( x, y )   ( ( y ) * VRAM_COLS + ( x ) + ( VRAM_FIRST_DATA >> 1 ) )
#define VRAM_CHARADDR8( x, y )  ( ( char* )vram_data + ( ( ( y ) * VRAM_COLS + ( x ) ) << 1 ) + VRAM_FIRST_DATA )
#define VRAM_MKCOL( fg, bg )         ( ( ( bg ) << 4 ) + fg )
#define VRAM_ANSI_ESC           0x1B

// *****************************************************************************
// ANSI sequence interpreter
// ANSI 'state machine'

static int vram_reading_ansi;
static int vram_ansi_count;
static char vram_ansi_inbuf[ TERM_MAX_ANSI_SIZE + 1 ];
// ANSI operations and structure data
enum
{
  ANSI_SEQ_CLRSCR,
  ANSI_SEQ_CLREOL,
  ANSI_SEQ_GOTOXY,
  ANSI_SEQ_UP,
  ANSI_SEQ_DOWN,
  ANSI_SEQ_RIGHT,
  ANSI_SEQ_LEFT,
  ANSI_SEQ_SETSGR,
  ANSI_SEQ_GOTOCOL
};

// ANSI SGR data
enum
{
  ANSI_SGR_RESET = 0,
  ANSI_SGR_BRIGHT,
  ANSI_SGR_FAINT,
  ANSI_SGR_FIRST_FGCOL = 30,
  ANSI_SGR_LAST_FGCOL = 37,
  ANSI_SGR_FIRST_BGCOL = 40,
  ANSI_SGR_LAST_BGCOL = 47
};

// Map ANSI colors to VRAM colors
static const u8 vram_ansi_col_lut[] = 
{
  VRAM_COL_BLACK, VRAM_COL_DARK_RED, VRAM_COL_DARK_GREEN, VRAM_COL_BROWN,
  VRAM_COL_DARK_BLUE, VRAM_COL_DARK_MAGENTA, VRAM_COL_DARK_CYAN, VRAM_COL_DARK_GRAY  
};
#define ANSI_COL_SIZE         ( sizeof( vram_ansi_col_lut ) / sizeof( u8 ) )
static u8 vram_ansi_brightness = ANSI_SGR_FAINT;

typedef struct
{
  int op;
  int p1, p2; 
} vram_ansi_op;

// Convert an ASCII escape sequence to an operation we can understand
static int vram_cvt_escape( char* inbuf, vram_ansi_op* res )
{
  const char *p = inbuf;
  char last = inbuf[ strlen( inbuf ) - 1 ];

  if( *p++ != '\x1B' )
    return 0;
  if( *p++ != '[' )
    return 0;
  res->op = res->p1 = res->p2 = -1;
  inbuf[ strlen( inbuf ) - 1 ] = '\0';
  switch( last )
  {
    case 'J': // clrscr
      if( *p != '2' )
        return 0;
      res->op = ANSI_SEQ_CLRSCR;
      break;

    case 'K': // clreol
      res->op = ANSI_SEQ_CLREOL;
      break;

    case 'H': // gotoxy
    case 'f':
      res->op = ANSI_SEQ_GOTOXY;
      if( last != 'H' )
        sscanf( p, "%d;%d", &res->p1, &res->p2 );
      else
        res->p1 = res->p2 = 1;
      break;

    case 'G': // go to column
      res->op = ANSI_SEQ_GOTOCOL;
      sscanf( p, "%d", &res->p1 );
      break;
      
    case 'm': // SGR
      res->op = ANSI_SEQ_SETSGR;
      if( strchr( p, ';' ) )
        sscanf( p, "%d;%d", &res->p1, &res->p2 );
      else
        sscanf( p, "%d", &res->p1 );
      break;

    case 'A': // up
    case 'B': // down
    case 'C': // right
    case 'D': // left
      res->op = last - 'A' + ANSI_SEQ_UP;
      sscanf( p, "%d", &res->p1 );
      break;

   default:
     return 0;
  }
  return 1;
}

static void vram_ansi_set_color( int fg, int bg )
{
  if( vram_ansi_brightness == ANSI_SGR_BRIGHT )
  {
    if( fg >= 0 )
      fg += ANSI_COL_SIZE;
    if( bg >= 0 )
      bg += ANSI_COL_SIZE;
  }
  vram_set_color( fg, bg );
}

static void vram_ansi_execute()
{
  vram_ansi_op op;
  
  if( vram_cvt_escape( vram_ansi_inbuf, &op ) )
  {
    // Interpret out sequence
    switch( op.op )
    {
      case ANSI_SEQ_CLRSCR:
        vram_clrscr();
        break;

      case ANSI_SEQ_CLREOL:
        vram_clreol();
        break;

      case ANSI_SEQ_GOTOXY:
        *vram_p_cx = ( u8 )op.p1 - 1;
        *vram_p_cy = ( u8 )op.p2 - 1;
        break;

      case ANSI_SEQ_GOTOCOL:
        *vram_p_cx = ( u8 )op.p1;        
        break;

      case ANSI_SEQ_UP:
      case ANSI_SEQ_LEFT:
      case ANSI_SEQ_RIGHT:
      case ANSI_SEQ_DOWN:
        {
          int xm = op.op == ANSI_SEQ_LEFT ? -1 : ( op.op == ANSI_SEQ_RIGHT ? 1 : 0 );
          int ym = op.op == ANSI_SEQ_UP ? -1 : ( op.op == ANSI_SEQ_DOWN ? 1 : 0 );
          vram_deltaxy( xm * op.p1, ym * op.p1 );
        }
        break;
        
      case ANSI_SEQ_SETSGR:
        {
          int ft, bt, r1, r2;
          if( op.p1 == ANSI_SGR_RESET )
          {
            ft = bt = VRAM_COL_DEFAULT;
            vram_ansi_brightness = ANSI_SGR_FAINT;
          }    
          else if( op.p1 == ANSI_SGR_BRIGHT || op.p1 == ANSI_SGR_FAINT )
          {
            ft = bt = VRAM_COL_DONT_CHANGE;
            vram_ansi_brightness = op.p1;
          }       
          else 
          {
            ft = bt = VRAM_COL_DONT_CHANGE;
            r1 = op.p1 >= ANSI_SGR_FIRST_FGCOL && op.p1 <= ANSI_SGR_LAST_FGCOL;
            r2 = op.p1 >= ANSI_SGR_FIRST_BGCOL && op.p1 <= ANSI_SGR_LAST_BGCOL;
            if( !r1 && !r2 )
              return;
            if( r2 )
              bt = vram_ansi_col_lut[ op.p1 - ANSI_SGR_FIRST_BGCOL ];
            else
              ft = vram_ansi_col_lut[ op.p1 - ANSI_SGR_FIRST_FGCOL ];
            r1 = op.p2 >= ANSI_SGR_FIRST_FGCOL && op.p2 <= ANSI_SGR_LAST_FGCOL;
            r2 = op.p2 >= ANSI_SGR_FIRST_BGCOL && op.p2 <= ANSI_SGR_LAST_BGCOL;
            if( r1 || r2 )
            {
              if( r2 )
                bt = vram_ansi_col_lut[ op.p2 - ANSI_SGR_FIRST_BGCOL ];
              else
                ft = vram_ansi_col_lut[ op.p2 - ANSI_SGR_FIRST_FGCOL ];
            }            
          }
          vram_ansi_set_color( ft, bt );
          break;
        }
     }
  }
}

// *****************************************************************************
// Helpers and local functions

static void vram_putchar_internal( int x, int y, char c )
{
  u16 *pdata = ( u16* )vram_data + VRAM_CHARADDR( x, y );
 
  *pdata = ( c << 8 ) | vram_crt_attr;
}

static void vram_clear_line( int y )
{
  u32 *pdata = ( u32* )vram_data + ( VRAM_CHARADDR( 0, y ) >> 1 );
  u32 fill = ( ' ' << 8 ) | vram_crt_attr;
  unsigned i;
  
  fill = ( fill << 16 ) | fill;
  for( i = 0; i < VRAM_COLS >> 1; i ++ )
    *pdata ++ = fill;   
}

// *****************************************************************************
// Public functions

void vram_init()
{
  u8 *pv = ( u8* )vram_data;
  
  // Initialize video memory contents
  vram_p_cx = pv + VRAM_OFF_CX;
  vram_p_cy = pv + VRAM_OFF_CY;
  vram_p_type = pv + VRAM_OFF_TYPE;
  *vram_p_type = VRAM_CURSOR_BLOCK_BLINK;
  vram_last_line = VRAM_LINES - 1;
  vram_set_color( VRAM_DEFAULT_FG_COL, VRAM_DEFAULT_BG_COL );
  vram_clrscr();  
} 

void vram_putchar( char c )
{
  unsigned i;
  
  // Take care of the ANSI state machine
  if( c == VRAM_ANSI_ESC )
  {
    vram_reading_ansi = 1;
    vram_ansi_count = 0;
    vram_ansi_inbuf[ vram_ansi_count ++ ] = c; 
    return;
  }  
  if( vram_reading_ansi )
  {
    vram_ansi_inbuf[ vram_ansi_count ++ ] = c;
    if( isalpha( c ) )
    {
      vram_ansi_inbuf[ vram_ansi_count ] = '\0';
      vram_ansi_execute();
      vram_reading_ansi = 0;
    }
    return;
  }  
  
  if( c == '\r' ) // Move cursor to the beginning of the line
    *vram_p_cx = 0;
  else if( c == '\n' )
  {
    *vram_p_cx = 0; // '\n' implies '\r'
    if( *vram_p_cy == vram_last_line )
    {
      // This is the last line and a NL was requested: shift the whole screen up one line
      memmove( ( u8* )vram_data + VRAM_FIRST_DATA, ( u8* )vram_data + VRAM_FIRST_DATA + VRAM_LINE_SIZE, vram_last_line * VRAM_LINE_SIZE );
      vram_clear_line( vram_last_line );
    }
    else
      *vram_p_cy += 1;
    if( vram_paging_enabled )
    {
      if( ++ vram_paging_lines == vram_last_line - 1 )
      {
        term_getch( TERM_INPUT_WAIT );
        vram_paging_lines = 0;
      }
    }
  }    
  else if( c == 8 ) // Backspace
  {
    if( *vram_p_cx == 0 )
    {
      if( *vram_p_cy > 0 )
      {
        *vram_p_cy -= 1;
        *vram_p_cx = VRAM_COLS - 1;        
      }
    }
    else
      *vram_p_cx -= 1;
    vram_putchar_internal( *vram_p_cx, *vram_p_cy, ' ' );
  }
  else if( c == '\t' ) // tab
  {
    for( i = 0; i < VRAM_TAB_SIZE; i ++ )
      vram_putchar( ' ' );    
  }
  else
  {
    if( *vram_p_cx >= VRAM_COLS )
    {
      // Last column on the line: write a new line (recursively) and change cursor position
      vram_putchar( '\n' );
      *vram_p_cx = 0;
    }
    vram_putchar_internal( *vram_p_cx, *vram_p_cy, c );  
    *vram_p_cx += 1;
  }
}            

void vram_send( int fd, char c )
{
  fd = fd;
  vram_putchar( c );
}

void vram_set_color( int fg, int bg )
{
  if( fg == VRAM_COL_DEFAULT )
    vram_fg_col = VRAM_DEFAULT_FG_COL;
  else if( fg != VRAM_COL_DONT_CHANGE )
    vram_fg_col = fg;    
  if( bg == VRAM_COL_DEFAULT )
    vram_bg_col = VRAM_DEFAULT_BG_COL;
  else if( bg != VRAM_COL_DONT_CHANGE )
    vram_bg_col = bg;
  vram_crt_attr = VRAM_MKCOL( vram_fg_col, vram_bg_col );
}

void vram_clrscr()
{
  unsigned i;
  u32 *pdata = vram_data + ( VRAM_FIRST_DATA >> 2 );
  u32 fill;
  
  fill = ( ' ' << 8 ) | vram_crt_attr;
  fill = ( fill << 16 ) | fill;
  for( i = 0; i < VRAM_SIZE_VMEM_ONLY >> 2; i ++ )
    *pdata ++ = fill;
  *vram_p_cx = 0;
  *vram_p_cy = 0; 
}

void vram_gotoxy( u8 x, u8 y )
{
  if( x < VRAM_COLS )
    *vram_p_cx = x;
  if( y < VRAM_LINES )
    *vram_p_cy = y;    
}

void vram_deltaxy( int dx, int dy )
{
  int x = *vram_p_cx;
  int y = *vram_p_cy;
  
  x += dx;
  y += dy;
  if( x > 0 && y > 0 )
    vram_gotoxy( x, y );
}

u8 vram_get_cx()
{
  return *vram_p_cx;
}

u8 vram_get_cy()
{
  return *vram_p_cy;
}

void vram_clreol()
{
  u16 *pdata = ( u16* )vram_data + VRAM_CHARADDR( *vram_p_cx, *vram_p_cy );
  u16 fill = ( ' ' << 8 ) | vram_crt_attr;
  unsigned i;
   
  for( i = *vram_p_cx; i < VRAM_COLS; i ++ )
    *pdata ++ = fill;
}

void vram_set_cursor( int type )
{
  switch( type )
  {
    case TERM_CURSOR_OFF:
      *vram_p_type = VRAM_CURSOR_OFF;
      break;

    case TERM_CURSOR_BLOCK:
      *vram_p_type = VRAM_CURSOR_BLOCK_BLINK;
      break;

    case TERM_CURSOR_UNDERLINE:
      *vram_p_type = VRAM_CURSOR_LINE_BLINK;
      break;
  }
}

int vram_get_cursor()
{
  switch( *vram_p_type )
  {
    case VRAM_CURSOR_OFF:
      return TERM_CURSOR_OFF;

    case VRAM_CURSOR_BLOCK_BLINK:
      return TERM_CURSOR_BLOCK;

    case VRAM_CURSOR_LINE_BLINK:
      return TERM_CURSOR_UNDERLINE;
  }
  return TERM_CURSOR_OFF;
}

// Enable/disable paging
void vram_enable_paging( int enabled )
{
  vram_paging_enabled = enabled;
  vram_paging_lines = 0;
}

// Draw a "box" (with an optional border an title) at the specified coordinates
// Return a "box id" that can be used later to close the box
void* vram_box( unsigned x, unsigned y, unsigned width, unsigned height, const char *title, u16 flags )
{
  TERM_BOX *pbox = NULL;
  unsigned ix, iy;
  u8 *crt;
  int hasborder = flags & TERM_BOX_FLAG_BORDER;

  if( ( pbox = ( TERM_BOX* )malloc( sizeof( TERM_BOX ) ) ) == NULL )
    goto error;
  memset( pbox, 0, sizeof( pbox ) );
  if( flags & TERM_BOX_FLAG_CENTER ) // recompute x and y to center the box properly
  {
    x = ( VRAM_COLS - width ) / 2;
    y = ( VRAM_LINES - height ) / 2;
  }
  if( flags & TERM_BOX_FLAG_RESTORE )
    if( ( pbox->savedata = ( u8* )malloc( width * height * 2 ) ) == NULL )
      goto error;
  pbox->x = ( u16 )x;
  pbox->y = ( u16 )y;
  pbox->width = ( u16 )width;
  pbox->height = ( u16 )height;
  pbox->flags = flags;
  // Save previous data from video RAM
  if( flags & TERM_BOX_FLAG_RESTORE )
    for( iy = y, crt = pbox->savedata; iy < y + height; iy ++ )
    {
      memcpy( crt, VRAM_CHARADDR8( x, iy ), width << 1 );
      crt += width << 1;
    }
  // Write the title line
  vram_putchar_internal( x, y, hasborder ? VRAM_SBOX_UL : ' ' );
  vram_putchar_internal( x + 1, y, hasborder ? VRAM_SBOX_HLINE : ' ' );
  vram_putchar_internal( x + 2, y, hasborder ? VRAM_SBOX_HLINE : ' ' );
  for( ix = 0; ix < ( title ? strlen( title ) : 0 ); ix ++ )
    vram_putchar_internal( x + 3 + ix, y, title[ ix ] );
  for( ix = ix + x + 3; ix < x + width - 1; ix ++ )
    vram_putchar_internal( ix, y, hasborder ? VRAM_SBOX_HLINE : ' ' );
  vram_putchar_internal( ix, y, hasborder ? VRAM_SBOX_UR : ' ' );
  // Write the body lines
  for( iy = y + 1; iy < y + height - 1; iy ++ )
  {
    vram_putchar_internal( x, iy, hasborder ? VRAM_SBOX_VLINE : ' ' );
    for( ix = x + 1; ix < x + width - 1; ix ++ )
      vram_putchar_internal( ix, iy, ' ' );
    vram_putchar_internal( ix, iy, hasborder ? VRAM_SBOX_VLINE : ' ' );
  }
  // Write the bottom line
  vram_putchar_internal( x, iy, hasborder ? VRAM_SBOX_BL : ' ' );
  for( ix = x + 1; ix < x + width - 1; ix ++ )
    vram_putchar_internal( ix, iy, hasborder ? VRAM_SBOX_HLINE : ' ' );
  vram_putchar_internal( ix, iy, hasborder ? VRAM_SBOX_BR : ' ' );
  // All done
  return pbox;
error:
  if( pbox && pbox->savedata )
    free( pbox->savedata );
  if( pbox )
    free( pbox );
  return NULL;
}

// Close that box
void vram_close_box( void *pbox )
{
  TERM_BOX *p = ( TERM_BOX* )pbox;
  u8 *crt;
  unsigned iy;

  // Restore video memory
  if( p->flags & TERM_BOX_FLAG_RESTORE )
    for( iy = p->y, crt = ( u8* )p->savedata; iy < p->y + p->height; iy ++ )
    {
      memcpy( VRAM_CHARADDR8( p->x, iy ), crt, p->width << 1 );
      crt += p->width << 1;
    }
  if( p->savedata )
    free( p->savedata );
  free( p );
}

void vram_get_color( int *pfgcol, int *pbgcol )  
{
  *pfgcol = vram_fg_col;
  *pbgcol = vram_bg_col;
}

void vram_set_last_line( int line )
{
  vram_last_line = line;
}

void vram_change_attr( unsigned x, unsigned y, unsigned len, int newfg, int newbg )
{
  char *pdata = VRAM_CHARADDR8( x, y );
  char newcol = VRAM_MKCOL( newfg, newbg );
  unsigned i;
  
  for( i = 0; i < len; i ++, pdata += 2 )
    *pdata = newcol;
}

