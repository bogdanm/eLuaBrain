// Module for interfacing with terminal functions

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "term.h"
#include "platform_conf.h"
#include "lrotable.h"
#include <string.h>
#include <stdlib.h>

// Lua: clrscr()
static int luaterm_clrscr( lua_State* L )
{
  term_clrscr();
  return 0;
}

// Lua: clreol()
static int luaterm_clreol( lua_State* L )
{
  term_clreol();
  return 0;
}

// Lua: moveto( x, y )
static int luaterm_moveto( lua_State* L )
{
  unsigned x, y;
  
  x = ( unsigned )luaL_checkinteger( L, 1 );
  y = ( unsigned )luaL_checkinteger( L, 2 );
  term_gotoxy( x, y );
  return 0;
}

// Lua: moveup( lines )
static int luaterm_moveup( lua_State* L )
{
  unsigned delta;
  
  delta = ( unsigned )luaL_checkinteger( L, 1 );
  term_up( delta );
  return 0;
}

// Lua: movedown( lines )
static int luaterm_movedown( lua_State* L )
{
  unsigned delta;
  
  delta = ( unsigned )luaL_checkinteger( L, 1 );
  term_down( delta );
  return 0;
}

// Lua: moveleft( cols )
static int luaterm_moveleft( lua_State* L )
{
  unsigned delta;
  
  delta = ( unsigned )luaL_checkinteger( L, 1 );
  term_left( delta );
  return 0;
}

// Lua: moveright( cols )
static int luaterm_moveright( lua_State* L )
{
  unsigned delta;
  
  delta = ( unsigned )luaL_checkinteger( L, 1 );
  term_right( delta );
  return 0;
}

// Lua: lines = getlines()
static int luaterm_getlines( lua_State* L )
{
  lua_pushinteger( L, term_get_lines() );
  return 1;
}

// Lua: columns = getcols()
static int luaterm_getcols( lua_State* L )
{
  lua_pushinteger( L, term_get_cols() );
  return 1;
}

// Lua: print( string1, string2, ... )
// or print( x, y, string1, string2, ... )
static int luaterm_print( lua_State* L )
{
  const char* buf;
  size_t len, i;
  int total = lua_gettop( L ), s = 1;
  int x = -1, y = -1;

  // Check if the function has integer arguments
  if( lua_isnumber( L, 1 ) && lua_isnumber( L, 2 ) )
  {
    x = lua_tointeger( L, 1 );
    y = lua_tointeger( L, 2 );
    term_gotoxy( x, y );
    s = 3;
  } 
  for( ; s <= total; s ++ )
  {
    luaL_checktype( L, s, LUA_TSTRING );
    buf = lua_tolstring( L, s, &len );
    for( i = 0; i < len; i ++ )
      term_putch( buf[ i ] );
  }
  return 0;
}

// Lua: cursorx = getcx()
static int luaterm_getcx( lua_State* L )
{
  lua_pushinteger( L, term_get_cx() );
  return 1;
}

// Lua: cursory = getcy()
static int luaterm_getcy( lua_State* L )
{
  lua_pushinteger( L, term_get_cy() );
  return 1;
}

// Lua: key = getchar( [ mode ] )
static int luaterm_getchar( lua_State* L )
{
  int temp = TERM_INPUT_WAIT;
  
  if( lua_isnumber( L, 1 ) )
    temp = lua_tointeger( L, 1 );
  lua_pushinteger( L, term_getch( temp ) );
  return 1;
}

// Lua: setcolor( [fgcol], [bgcol] )
static int luaterm_setcolor( lua_State *L )
{
  int fgcol, bgcol;

  if( lua_gettop( L ) == 0 )
    fgcol = bgcol = TERM_COL_DEFAULT;
  else
  {
    fgcol = ( int )luaL_checkinteger( L, 1 );
    bgcol = ( int )luaL_checkinteger( L, 2 );
  }
  term_set_color( fgcol, bgcol );
  return 0;
}

// Lua: setfg( fgcol )
static int luaterm_setfg( lua_State *L )
{
  int fgcol = ( int )luaL_checkinteger( L, 1 );

  term_set_color( fgcol, TERM_COL_DONT_CHANGE );
  return 0;
}

// Lua: setbg( fgcol )
static int luaterm_setbg( lua_State *L )
{
  int bgcol = ( int )luaL_checkinteger( L, 1 );

  term_set_color( TERM_COL_DONT_CHANGE, bgcol );
  return 0;
}

// Lua: setcursor( type )
static int luaterm_setcursor( lua_State *L )
{
  int type = luaL_checkinteger( L, 1 );

  term_set_cursor( type );
  return 0;
}

// Lua: reset()
static int luaterm_reset( lua_State *L )
{
  term_reset();
  return 0;
}

// Lua: id = box( x, y, width, height, title, attrs )
static int luaterm_box( lua_State *L )
{
  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );
  int width = luaL_checkinteger( L, 3 );
  int height = luaL_checkinteger( L, 4 );
  const char *title = luaL_checkstring( L, 5 );
  int attrs = luaL_checkinteger( L, 6 );
  void *p;

  p = term_box( x, y, width, height, title, attrs );
  lua_pushlightuserdata( L, p );
  return 1;
}

// Lua: close_box( id )
static int luaterm_close_box( lua_State *L )
{
  void *p;

  if( lua_type( L, 1 ) != LUA_TLIGHTUSERDATA )
    return luaL_error( L, "invalid argument" );
  p = lua_touserdata( L, 1 );
  term_close_box( p );
  return 0;
}

// Lua: setpaging( [bool] )
static int luaterm_setpaging( lua_State *L )
{
  int mode = TERM_PAGING_OFF;

  if( lua_type( L, 1 ) == LUA_TBOOLEAN )
    mode = lua_toboolean( L, 1 );
  term_enable_paging( mode );
  return 0;
}

// Lua: getstr( x, y, prompt, maxlen, [validator] )
static int luaterm_getstr( lua_State *L )
{
  int cx = luaL_checkinteger( L, 1 );
  int cy = luaL_checkinteger( L, 2 );
  const char *prompt = lua_isnil( L, 3 ) ? NULL : luaL_checkstring( L, 3 );
  int maxlen = luaL_checkinteger( L, 4 );
  int has_validator = lua_isfunction( L, 5 );
  int entered = 0, c, vflag;
  char *input;
  char current[ 2 ];

  if( ( input = ( char* )malloc( maxlen + 1 ) ) == NULL )
    return luaL_error( L, "not enough memory" );
  term_gotoxy( cx, cy );
  if( prompt )
  {
    term_cstr( prompt );
    cx += strlen( prompt );
  }
  input[ 0 ] = '\0';
  current[ 1 ]  = '\0';
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
      break;
    else if ( c == KC_ESC ) // Escape
    {
      free( input );
      input = NULL;
      break;
    }
    else if( c < TERM_FIRST_KEY && strlen( input ) < maxlen ) // regular ASCII char, ignore everything else
    {
      if( !has_validator )
        vflag = 1;
      else // call validator with current string and last char
      {
        lua_pushstring( L, input );
        current[ 0 ] = c;
        lua_pushstring( L, current );
        lua_call( L, 2, 1 );
        vflag = lua_isboolean( L, -1 ) ? lua_toboolean( L, -1 ) : 0;
        lua_pop( L, 1 );
      }
      if( vflag )
      {
        term_putch( c );
        term_gotoxy( ++ cx, cy );
        input[ entered ++ ] = c;
        input[ entered ] = '\0';
      }
    }
  }
  if( input )
  {
    lua_pushstring( L, input );
    free( input );
  }
  else
    lua_pushnil( L );
  return 1;
}

// Lua: set_last_line( line )
static int luaterm_set_last_line( lua_State *L )
{
  int line = luaL_checkinteger( L, 1 );

  term_set_last_line( line );
  return 0;
}

// Key codes by name
#undef _D
#define _D( x ) #x
static const char* term_key_names[] = { TERM_KEYCODES };

// __index metafunction for term
// Look for all KC_xxxx codes
static int term_mt_index( lua_State* L )
{
  const char *key = luaL_checkstring( L ,2 );
  unsigned i, total = sizeof( term_key_names ) / sizeof( char* );
  
  if( !key || *key != 'K' )
    return 0;
  for( i = 0; i < total; i ++ )
    if( !strcmp( key, term_key_names[ i ] ) )
      break;
  if( i == total )
    return 0;
  else
  {
    lua_pushinteger( L, i + TERM_FIRST_KEY );
    return 1; 
  }
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
#define COLLINE( c )          { LSTRKEY( #c ), LNUMVAL( TERM_##c ) }
const LUA_REG_TYPE term_map[] = 
{
  { LSTRKEY( "clrscr" ), LFUNCVAL( luaterm_clrscr ) },
  { LSTRKEY( "clreol" ), LFUNCVAL( luaterm_clreol ) },
  { LSTRKEY( "moveto" ), LFUNCVAL( luaterm_moveto ) },
  { LSTRKEY( "moveup" ), LFUNCVAL( luaterm_moveup ) },
  { LSTRKEY( "movedown" ), LFUNCVAL( luaterm_movedown ) },
  { LSTRKEY( "moveleft" ), LFUNCVAL( luaterm_moveleft ) },
  { LSTRKEY( "moveright" ), LFUNCVAL( luaterm_moveright ) },
  { LSTRKEY( "getlines" ), LFUNCVAL( luaterm_getlines ) },
  { LSTRKEY( "getcols" ), LFUNCVAL( luaterm_getcols ) },
  { LSTRKEY( "print" ), LFUNCVAL( luaterm_print ) },
  { LSTRKEY( "getcx" ), LFUNCVAL( luaterm_getcx ) },
  { LSTRKEY( "getcy" ), LFUNCVAL( luaterm_getcy ) },
  { LSTRKEY( "getchar" ), LFUNCVAL( luaterm_getchar ) },
  { LSTRKEY( "setcolor" ), LFUNCVAL( luaterm_setcolor ) },
  { LSTRKEY( "setfg" ), LFUNCVAL( luaterm_setfg ) },
  { LSTRKEY( "setbg" ), LFUNCVAL( luaterm_setbg ) },
  { LSTRKEY( "setcursor" ), LFUNCVAL( luaterm_setcursor ) },
  { LSTRKEY( "reset" ), LFUNCVAL( luaterm_reset ) },
  { LSTRKEY( "box" ), LFUNCVAL( luaterm_box ) },
  { LSTRKEY( "close_box" ), LFUNCVAL( luaterm_close_box ) },
  { LSTRKEY( "setpaging" ), LFUNCVAL( luaterm_setpaging ) },
  { LSTRKEY( "getstr" ), LFUNCVAL( luaterm_getstr ) },
  { LSTRKEY( "set_last_line" ), LFUNCVAL( luaterm_set_last_line ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "__metatable" ), LROVAL( term_map ) },
  { LSTRKEY( "NOWAIT" ), LNUMVAL( TERM_INPUT_DONT_WAIT ) },
  { LSTRKEY( "WAIT" ), LNUMVAL( TERM_INPUT_WAIT ) },
  { LSTRKEY( "COL_DONT_CHANGE" ), LNUMVAL( TERM_COL_DONT_CHANGE ) },
  { LSTRKEY( "COL_DEFAULT" ), LNUMVAL( TERM_COL_DEFAULT ) },  
  { LSTRKEY( "BOX_RESTORE" ), LNUMVAL( TERM_BOX_FLAG_RESTORE ) },
  { LSTRKEY( "BOX_BORDER" ), LNUMVAL( TERM_BOX_FLAG_BORDER ) },
  COLLINE( COL_BLACK ),
  COLLINE( COL_DARK_BLUE ),
  COLLINE( COL_DARK_GREEN ),
  COLLINE( COL_DARK_CYAN ),
  COLLINE( COL_DARK_RED ),
  COLLINE( COL_DARK_MAGENTA ),
  COLLINE( COL_BROWN ),
  COLLINE( COL_LIGHT_GRAY ),
  COLLINE( COL_DARK_GRAY ),
  COLLINE( COL_LIGHT_BLUE ),
  COLLINE( COL_LIGHT_GREEN ),
  COLLINE( COL_LIGHT_CYAN ),
  COLLINE( COL_LIGHT_RED ),
  COLLINE( COL_LIGHT_MAGENTA ),
  COLLINE( COL_LIGHT_YELLOW ),
  COLLINE( COL_WHITE ),  
  COLLINE( CURSOR_OFF ),
  COLLINE( CURSOR_BLOCK ),
  COLLINE( CURSOR_UNDERLINE ),
#endif
  { LSTRKEY( "__index" ), LFUNCVAL( term_mt_index ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_term( lua_State* L )
{
#ifdef BUILD_TERM
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  // Register methods
  luaL_register( L, AUXLIB_TERM, term_map );  
  
  // Set this table as itw own metatable
  lua_pushvalue( L, -1 );
  lua_setmetatable( L, -2 );  
  
  // Register the constants for "getch"
  lua_pushnumber( L, TERM_INPUT_DONT_WAIT );
  lua_setfield( L, -2, "NOWAIT" );  
  lua_pushnumber( L, TERM_INPUT_WAIT );
  lua_setfield( L, -2, "WAIT" );  
  
  return 1;
#endif // # if LUA_OPTIMIZE_MEMORY > 0
#else // #ifdef BUILD_TERM
  return 0;
#endif // #ifdef BUILD_TERM  
}

