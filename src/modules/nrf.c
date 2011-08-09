// Module for interfacing with the nRF radio interface

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include "nrf.h"
#include <string.h>

// Helper: parse an address, return 1 if OK or 0 if error
static int nrfmh_parse_address( const char *addr, u8 *pdest )
{
  int sz;

  if( sscanf( addr, "%2hhX:%2hhX:%2hhX:%2hhX:%2hhX%n", pdest, pdest + 1, pdest + 2, pdest + 3, pdest + 4, &sz ) != 5 || sz != strlen( addr ) )
    return 0;
  return 1;
}

// Lua: set_mode( nrf.TX | nrf.RX )
static int nrfm_set_mode( lua_State *L )
{
  int mode = luaL_checkinteger( L, 1 );

  nrf_init();
  nrf_set_mode( mode );
  return 0;
}

// Lua: sentbytes = write( address, data )
// 'Address' is in hex format delimited by ':' (aa:bb:cc:dd:ee)
// 'data' is a string
static int nrfm_write( lua_State *L )
{
  const char *addr = luaL_checkstring( L, 1 );
  u8 destaddr[ 5 ];
  size_t len;
  const char *data;
  
  nrf_init();
  if( !nrfmh_parse_address( addr, destaddr ) )
    return luaL_error( L, "invalid address" );
  luaL_checkstring( L, 2 );
  data = lua_tolstring( L, 2, &len );
  if( len > NRF_MAX_PAYLOAD_SIZE || len == 0 )
    return luaL_error( L, "data to send is too large (maximum is %d bytes) or empty", NRF_MAX_PAYLOAD_SIZE );
  lua_pushinteger( L, nrf_send_packet( destaddr, ( u8* )data, len ) );
  return 1;
}

// Lua: data = read( [timeout], [timer_id] )
static int nrfm_read( lua_State *L )
{
  int timeout = luaL_optinteger( L, 1, PLATFORM_UART_INFINITE_TIMEOUT );
  int timer_id = luaL_optinteger( L, 2, -1 );
  luaL_Buffer b;
  u8 data[ NRF_MAX_PAYLOAD_SIZE ];
  unsigned len = 0;
  timer_data_type tmr_start, tmr_crt;

  nrf_init();
  luaL_buffinit( L, &b );
  if( timeout == 0 )
    len = nrf_get_packet( data, NRF_MAX_PAYLOAD_SIZE, NULL );
  else if( timeout == PLATFORM_UART_INFINITE_TIMEOUT )
    while( ( len = nrf_get_packet( data, NRF_MAX_PAYLOAD_SIZE, NULL ) ) == 0 );
  else
  {
    if( timer_id == -1 )
      return luaL_error( L, "timer not specified" );
    // Receive data with the specified timeout
    tmr_start = platform_timer_op( timer_id, PLATFORM_TIMER_OP_START, 0 );
    while( 1 )
    {
      if( ( len = nrf_get_packet( data, NRF_MAX_PAYLOAD_SIZE, NULL ) ) > 0 )
        break;
      tmr_crt = platform_timer_op( timer_id, PLATFORM_TIMER_OP_READ, 0 );
      if( platform_timer_get_diff_us( timer_id, tmr_crt, tmr_start ) >= timeout )
        break;
    }
  }
  if( len > 0 )
    luaL_addlstring( &b, ( const char* )data, len );
  luaL_pushresult( &b );
  return 1;
}

// Lua: set_address( address )
static int nrfm_set_address( lua_State *L )
{
  const char *addr = luaL_checkstring( L, 1 );
  u8 pipeaddr[ 5 ];

  nrf_init();
  if( !nrfmh_parse_address( addr, pipeaddr ) )
    return luaL_error( L, "invalid address" );
  nrf_set_own_addr( pipeaddr );
  return 0;
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE nrf_map[] = 
{
  { LSTRKEY( "set_mode" ), LFUNCVAL( nrfm_set_mode ) },
  { LSTRKEY( "write" ), LFUNCVAL( nrfm_write ) },
  { LSTRKEY( "read" ), LFUNCVAL( nrfm_read ) },
  { LSTRKEY( "set_address" ), LFUNCVAL( nrfm_set_address ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "RX" ), LNUMVAL( NRF_MODE_RX ) },
  { LSTRKEY( "TX" ), LNUMVAL( NRF_MODE_TX ) },
  { LSTRKEY( "NO_TIMEOUT" ), LNUMVAL( 0 ) },
  { LSTRKEY( "INF_TIMEOUT" ), LNUMVAL( PLATFORM_UART_INFINITE_TIMEOUT ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_nrf( lua_State* L )
{
  LREGISTER( L, AUXLIB_NRF, nrf_map );
}

