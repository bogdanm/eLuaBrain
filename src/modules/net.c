// Module for interfacing with network functions (elua_net.h)

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "elua_net.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "lrotable.h"

#include "platform_conf.h"
#ifdef BUILD_UIP

#define lua_puship( L, ip )   lua_pushnumber( L, ( lua_Number )ip )

// Lua: sock, remoteip, err = accept( port, [ timer_id, timeout ] )
static int net_accept( lua_State *L )
{
  u16 port = ( u16 )luaL_checkinteger( L, 1 );
  unsigned timer_id = 0;
  s32 timeout = ELUA_NET_INF_TIMEOUT;
  elua_net_ip remip;
  int sock;

  if( lua_gettop( L ) >= 2 ) // check for timeout arguments
  {
    timer_id = ( unsigned )luaL_checkinteger( L, 2 );
    timeout = ( s32 )luaL_checkinteger( L, 3 );
  }  
  lua_pushinteger( L, sock = elua_net_accept( port, timer_id, timeout, &remip ) );
  lua_puship( L, remip.ipaddr );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  return 3;
}

// Lua: sock = socket( type )
static int net_socket( lua_State *L )
{
  int type = ( int )luaL_checkinteger( L, 1 );
  
  lua_pushinteger( L, elua_net_socket( type ) );
  return 1;
}

// Lua: set_buffer( sock, size )
// Use '0' to disable buffering
static int net_set_buffer( lua_State *L )
{
  int sock = luaL_checkinteger( L, 1 );
  unsigned bufsize = ( unsigned )luaL_checkinteger( L, 2 );
  
  if( elua_net_set_buffer( sock, bufsize ) == 0 )
    return luaL_error( L, "unable to set buffer" );
  return 0;
}

// Lua: set_split( sock, [split_char])
// Defaults to 'no split' if not specified
static int net_set_split( lua_State *L )
{
  int sock = luaL_checkinteger( L, 1 );
  int schar = luaL_optinteger( L, 2, ELUA_NET_NO_SPLIT );

  lua_pushinteger( L, elua_net_set_split( sock, schar ) );
  return 1;
}

// Lua: res = close( socket )
static int net_close( lua_State* L )
{
  int sock = ( int )luaL_checkinteger( L, 1 );
  
  lua_pushinteger( L, elua_net_close( sock ) );
  return 1;
}

// Lua: res, err = send( sock, str )
static int net_send( lua_State* L )
{
  int sock = ( int )luaL_checkinteger( L, 1 );
  const char *buf;
  size_t len;
    
  luaL_checktype( L, 2, LUA_TSTRING );
  buf = lua_tolstring( L, 2, &len );
  lua_pushinteger( L, elua_net_send( sock, buf, len ) );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  return 2;  
}

// Lua: err = connect( sock, iptype, port )
// "iptype" is actually an int returned by "net.packip"
static int net_connect( lua_State *L )
{
  elua_net_ip ip;
  int sock = ( int )luaL_checkinteger( L, 1 );
  u16 port = ( u16 )luaL_checkinteger( L, 3 );
  
  luaL_checkinteger( L, 2 );
  ip.ipaddr = ( u32 )luaL_checknumber( L, 2 );
  elua_net_connect( sock, ip, port );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  return 1;  
}

// Lua: data = packip( ip0, ip1, ip2, ip3 ), or
// Lua: data = packip( "ip" )
// Returns an internal representation for the given IP address
static int net_packip( lua_State *L )
{
  elua_net_ip ip;
  unsigned i, temp;
  
  if( lua_isnumber( L, 1 ) )
    for( i = 0; i < 4; i ++ )
    {
      temp = luaL_checkinteger( L, i + 1 );
      if( temp < 0 || temp > 255 )
        return luaL_error( L, "invalid IP adddress" );
      ip.ipbytes[ i ] = temp;
    }
  else
  {
    const char* pip = luaL_checkstring( L, 1 );
    unsigned len, temp[ 4 ];
    
    if( sscanf( pip, "%u.%u.%u.%u%n", temp, temp + 1, temp + 2, temp + 3, &len ) != 4 || len != strlen( pip ) )
      return luaL_error( L, "invalid IP adddress" );    
    for( i = 0; i < 4; i ++ )
    {
      if( temp[ i ] < 0 || temp[ i ] > 255 )
        return luaL_error( L, "invalid IP address" );
      ip.ipbytes[ i ] = ( u8 )temp[ i ];
    }
  }
  lua_puship( L, ip.ipaddr );
  return 1;
}

// Lua: ip0, ip1, ip2, ip3 = unpackip( iptype, "*n" ), or
//               string_ip = unpackip( iptype, "*s" )
static int net_unpackip( lua_State *L )
{
  elua_net_ip ip;
  unsigned i;  
  const char* fmt;
  
  luaL_checkinteger( L, 1 );
  ip.ipaddr = ( u32 )luaL_checknumber( L, 1 );
  fmt = luaL_checkstring( L, 2 );
  if( !strcmp( fmt, "*n" ) )
  {
    for( i = 0; i < 4; i ++ ) 
      lua_pushinteger( L, ip.ipbytes[ i ] );
    return 4;
  }
  else if( !strcmp( fmt, "*s" ) )
  {
    lua_pushfstring( L, "%d.%d.%d.%d", ( int )ip.ipbytes[ 0 ], ( int )ip.ipbytes[ 1 ], 
                     ( int )ip.ipbytes[ 2 ], ( int )ip.ipbytes[ 3 ] );
    return 1;
  }
  else
    return luaL_error( L, "invalid format" );                                      
}

// Lua: res, err = recv( sock, maxsize, [ timer_id, timeout ] )
static int net_recv( lua_State *L )
{
  int sock = ( int )luaL_checkinteger( L, 1 );
  elua_net_size maxsize = ( elua_net_size )luaL_checkinteger( L, 2 );
  unsigned timer_id = 0;
  s32 timeout = ELUA_NET_INF_TIMEOUT;
  luaL_Buffer net_recv_buff;

  if( lua_gettop( L ) >= 3 ) // check for timeout arguments
  {
    timer_id = ( unsigned )luaL_checkinteger( L, 3 );
    timeout = ( s32 )luaL_checkinteger( L, 4 );
  }
  // Initialize buffer
  luaL_buffinit( L, &net_recv_buff );
  elua_net_recvbuf( sock, &net_recv_buff, maxsize, timer_id, timeout );
  luaL_pushresult( &net_recv_buff );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  return 2;
}

// Lua: iptype = lookup( "name" )
static int net_lookup( lua_State* L )
{
  const char* name = luaL_checkstring( L, 1 );
  elua_net_ip res;
  
  res = elua_net_lookup( name );
  lua_puship( L, res.ipaddr );
  return 1;
}

// Lua: ip, netmask, dns, gw = netcfg()
// Returns 'nil' if the network isn't initialized yet
static int net_netcfg( lua_State *L )
{
  elua_net_ip ip;

  ip = elua_net_get_config( ELUA_NET_CFG_IP );
  if( ip.ipaddr == 0 )
    return 0;
  lua_puship( L, ip.ipaddr );
  ip = elua_net_get_config( ELUA_NET_CFG_NETMASK );
  lua_puship( L, ip.ipaddr );
  ip = elua_net_get_config( ELUA_NET_CFG_DNS );
  lua_puship( L, ip.ipaddr );
  ip = elua_net_get_config( ELUA_NET_CFG_GW );
  lua_puship( L, ip.ipaddr );
  return 4;
}

// Lua: res, err = sendto( socket, data, remoteip, remoteport )
static int net_sendto( lua_State *L )
{
  int sock = luaL_checkinteger( L, 1 );
  u16 port = ( u16 )luaL_checkinteger( L, 4 );
  elua_net_ip ip;
  const char *buf;
  size_t len;
    
  luaL_checktype( L, 2, LUA_TSTRING );
  buf = lua_tolstring( L, 2, &len );
  luaL_checkinteger( L, 3 );
  ip.ipaddr = ( u32 )luaL_checknumber( L, 3 );
  lua_pushinteger( L, elua_net_sendto( sock, buf, len, ip, port ) );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  return 2; 
}

// Lua: res, err, remoteip, remoteport = recvfrom( sock, maxsize, [ timer_id, timeout ] )
static int net_recvfrom( lua_State *L )
{
  int sock = ( int )luaL_checkinteger( L, 1 );
  elua_net_size maxsize = ( elua_net_size )luaL_checkinteger( L, 2 );
  unsigned timer_id = 0;
  s32 timeout = ELUA_NET_INF_TIMEOUT;
  luaL_Buffer net_recv_buff;
  elua_net_ip remoteip;
  u16 remoteport;

 if( lua_gettop( L ) >= 3 ) // check for timeout arguments
  {
    timer_id = ( unsigned )luaL_checkinteger( L, 3 );
    timeout = ( s32 )luaL_checkinteger( L, 4 );
  }
  // Initialize buffer
  luaL_buffinit( L, &net_recv_buff );
  elua_net_recvfrombuf( sock, &net_recv_buff, maxsize, &remoteip, &remoteport, timer_id, timeout );
  luaL_pushresult( &net_recv_buff );
  lua_pushinteger( L, elua_net_get_last_err( sock ) );
  lua_puship( L, remoteip.ipaddr );
  lua_pushinteger( L, remoteport );
  return 4;
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE net_map[] = 
{
  { LSTRKEY( "accept" ), LFUNCVAL( net_accept ) },
  { LSTRKEY( "packip" ), LFUNCVAL( net_packip ) },
  { LSTRKEY( "unpackip" ), LFUNCVAL( net_unpackip ) },
  { LSTRKEY( "connect" ), LFUNCVAL( net_connect ) },
  { LSTRKEY( "socket" ), LFUNCVAL( net_socket ) },
  { LSTRKEY( "set_buffer" ), LFUNCVAL( net_set_buffer ) },
  { LSTRKEY( "set_split" ), LFUNCVAL( net_set_split ) },
  { LSTRKEY( "close" ), LFUNCVAL( net_close ) },
  { LSTRKEY( "send" ), LFUNCVAL( net_send ) },
  { LSTRKEY( "recv" ), LFUNCVAL( net_recv ) },
  { LSTRKEY( "lookup" ), LFUNCVAL( net_lookup ) },
  { LSTRKEY( "netcfg" ), LFUNCVAL( net_netcfg ) },
  { LSTRKEY( "sendto" ), LFUNCVAL( net_sendto ) },
  { LSTRKEY( "recvfrom" ), LFUNCVAL( net_recvfrom ) },
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "SOCK_STREAM" ), LNUMVAL( ELUA_NET_SOCK_STREAM ) },
  { LSTRKEY( "SOCK_DGRAM" ), LNUMVAL( ELUA_NET_SOCK_DGRAM ) },
  { LSTRKEY( "ERR_OK" ), LNUMVAL( ELUA_NET_ERR_OK ) },
  { LSTRKEY( "ERR_TIMEOUT" ), LNUMVAL( ELUA_NET_ERR_TIMEDOUT ) },
  { LSTRKEY( "ERR_CLOSED" ), LNUMVAL( ELUA_NET_ERR_CLOSED ) },
  { LSTRKEY( "ERR_ABORTED" ), LNUMVAL( ELUA_NET_ERR_ABORTED ) },
  { LSTRKEY( "ERR_OVERFLOW" ), LNUMVAL( ELUA_NET_ERR_OVERFLOW ) },
  { LSTRKEY( "INVALID_SOCKET" ), LNUMVAL( ELUA_NET_INVALID_SOCKET ) },
  { LSTRKEY( "INF_TIMEOUT" ), LNUMVAL( ELUA_NET_INF_TIMEOUT ) },
  { LSTRKEY( "NO_SPLIT" ), LNUMVAL( ELUA_NET_NO_SPLIT ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_net( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_NET, net_map );  

  // Module constants  
  MOD_REG_NUMBER( L, "SOCK_STREAM", ELUA_NET_SOCK_STREAM );
  MOD_REG_NUMBER( L, "SOCK_DGRAM", ELUA_NET_SOCK_DGRAM ); 
  MOD_REG_NUMBER( L, "ERR_OK", ELUA_NET_ERR_OK );
  MOD_REG_NUMBER( L, "ERR_TIMEDOUT", ELUA_NET_ERR_TIMEDOUT );
  MOD_REG_NUMBER( L, "ERR_CLOSED", ELUA_NET_ERR_CLOSED );
  MOD_REG_NUMBER( L, "ERR_ABORTED", ELUA_NET_ERR_ABORTED );
  MOD_REG_NUMBER( L, "ERR_OVERFLOW", ELUA_NET_ERR_OVERFLOW );
  MOD_REG_NUMBER( L, "INVALID_SOCKET", ELUA_NET_INVALID_SOCKET );
  MOD_REG_NUMBER( L, "INF_TIMEOUT", ELUA_NET_INF_TIMEOUT );
  MOD_REG_NUMEBR( L, "NO_SPLIT", ELUA_NET_NO_SPLIT );
  
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}

#else // #ifdef BUILD_UIP

LUALIB_API int luaopen_net( lua_State *L )
{
  return 0;
}

#endif // #ifdef BUILD_UIP

