#include "platform_conf.h"
#ifdef BUILD_UIP

// UIP "helper" for eLua
// Implements the eLua specific UIP application

#include "elua_uip.h"
#include "elua_net.h"
#include "type.h"
#include "uip.h"
#include "uip_arp.h"
#include "platform.h"
#include "utils.h"
#include "uip-split.h"
#include "dhcpc.h"
#include "resolv.h"
#include <string.h>
#include <stdio.h>

// UIP send buffer
extern void* uip_sappdata;

// Global "configured" flag
static volatile u8 elua_uip_configured;

// *****************************************************************************
// Platform independenet eLua UIP "main loop" implementation

// Timers
static u32 periodic_timer, arp_timer;

// Macro for accessing the Ethernet header information in the buffer.
#define BUF                     ((struct uip_eth_hdr *)&uip_buf[0])
#define UDPBUF                  ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

// UIP Timers (in ms)
#define UIP_PERIODIC_TIMER_MS   500
#define UIP_ARP_TIMER_MS        10000

#define IP_TCP_HEADER_LENGTH 40
#define TOTAL_HEADER_LENGTH (IP_TCP_HEADER_LENGTH+UIP_LLH_LEN)

static void device_driver_send()
{
  platform_eth_send_packet( uip_buf, uip_len );
/*  if( uip_len <= TOTAL_HEADER_LENGTH )
    platform_eth_send_packet( uip_buf, uip_len );
  else
  {
    platform_eth_send_packet( uip_buf, TOTAL_HEADER_LENGTH );
    platform_eth_send_packet( ( u8* )uip_appdata, uip_len - TOTAL_HEADER_LENGTH );
  } */
}

// This gets called on both Ethernet RX interrupts and timer requests,
// but it's called only from the Ethernet interrupt handler
void elua_uip_mainloop()
{
  u32 temp, packet_len;

  // Increment uIP timers
  temp = platform_eth_get_elapsed_time();
  periodic_timer += temp;
  arp_timer += temp;  

  // Check for an RX packet and read it
  if( ( packet_len = platform_eth_get_packet_nb( uip_buf, sizeof( uip_buf ) ) ) > 0 )
  {
    // Set uip_len for uIP stack usage.
    uip_len = ( unsigned short )packet_len;

    // Process incoming IP packets here.
    if( BUF->type == htons( UIP_ETHTYPE_IP ) )
    {
      uip_arp_ipin();
      uip_input();

      // If the above function invocation resulted in data that
      // should be sent out on the network, the global variable
      // uip_len is set to a value > 0.
      if( uip_len > 0 )
      {
        uip_arp_out();
        device_driver_send();
      }
    }

    // Process incoming ARP packets here.
    else if( BUF->type == htons( UIP_ETHTYPE_ARP ) )
    {
      uip_arp_arpin();

      // If the above function invocation resulted in data that
      // should be sent out on the network, the global variable
      // uip_len is set to a value > 0.
      if( uip_len > 0 )
        device_driver_send();
    }
  }
  
  // Process TCP/IP Periodic Timer here.
  // Also process the "force interrupt" events (platform_eth_force_interrupt)
  if( periodic_timer >= UIP_PERIODIC_TIMER_MS )
  {
    periodic_timer = 0;
    uip_set_forced_poll( 0 );
  }
  else
    uip_set_forced_poll( 1 );
  for( temp = 0; temp < UIP_CONNS; temp ++ )
  {
    uip_periodic( temp );

    // If the above function invocation resulted in data that
    // should be sent out on the network, the global variable
    // uip_len is set to a value > 0.
    if( uip_len > 0 )
    {
      if( uip_arp_out() ) // packet was replaced with ARP, need to resend
        uip_conns[ temp ].appstate.state = ELUA_UIP_STATE_RETRY;
      device_driver_send();
    }
  }

#if UIP_UDP
  for( temp = 0; temp < UIP_UDP_CONNS; temp ++ )
  {
    uip_udp_periodic( temp );

    // If the above function invocation resulted in data that
    // should be sent out on the network, the global variable
    // uip_len is set to a value > 0.
    if( uip_len > 0 )
    {
      if( uip_arp_out() ) // packet was replaced with ARP, need to resend
        uip_udp_conns[ temp ].appstate.state = ELUA_UIP_STATE_RETRY;
      device_driver_send();
    }
  }
#endif // UIP_UDP
  
  // Process ARP Timer here.
  if( arp_timer >= UIP_ARP_TIMER_MS )
  {
    arp_timer = 0;
    uip_arp_timer();
  }  
}

// *****************************************************************************
// DHCP callback

#ifdef BUILD_DHCPC
static void elua_uip_conf_static();

void dhcpc_configured(const struct dhcpc_state *s)
{
  if( s->ipaddr[ 0 ] != 0 )
  {
    printf( "GOT DHCP IP!!!\n" );
    uip_sethostaddr( s->ipaddr );
    uip_setnetmask( s->netmask ); 
    uip_setdraddr( s->default_router );     
    resolv_conf( ( u16_t* )s->dnsaddr );
    elua_uip_configured = 1;
  }
  else
    elua_uip_conf_static();
}
#endif

// *****************************************************************************
// DNS callback

#ifdef BUILD_DNS
volatile static int elua_resolv_req_done;
static elua_net_ip elua_resolv_ip;

void resolv_found( char *name, u16_t *ipaddr )
{
  if( !ipaddr )
    elua_resolv_ip.ipaddr = 0;
  else
  {
    elua_resolv_ip.ipwords[ 0 ] = ipaddr[ 0 ];
    elua_resolv_ip.ipwords[ 1 ] = ipaddr[ 1 ];
  }
  elua_resolv_req_done = 1;
}
#endif

// *****************************************************************************
// Console over Ethernet support

#ifdef BUILD_CON_TCP

// TELNET specific data
#define TELNET_IAC_CHAR        255
#define TELNET_IAC_3B_FIRST    251
#define TELNET_IAC_3B_LAST     254
#define TELNET_SB_CHAR         250
#define TELNET_SE_CHAR         240
#define TELNET_EOF             236

// The telnet socket number
static int elua_uip_telnet_socket = -1;

// Utility function for TELNET: parse input buffer, skipping over
// TELNET specific sequences
// Returns the length of the buffer after processing
static void elua_uip_telnet_handle_input( volatile struct elua_uip_state* s )
{
  u8 *dptr = ( u8* )uip_appdata;
  char *orig = ( char* )s->ptr;
  int skip;
  elua_net_size maxsize = s->len;
  
  // Traverse the input buffer, skipping over TELNET sequences
  while( ( dptr < ( u8* )uip_appdata + uip_datalen() ) && ( s->ptr - orig < s->len ) )
  {
    if( *dptr != TELNET_IAC_CHAR ) // regular char, copy it to buffer
      *s->ptr ++ = *dptr ++;
    else
    {
      // Control sequence: 2 or 3 bytes?
      if( ( dptr[ 1 ] >= TELNET_IAC_3B_FIRST ) && ( dptr[ 1 ] <= TELNET_IAC_3B_LAST ) )
        skip = 3;
      else
      {
        // Check EOF indication
        if( dptr[ 1 ] == TELNET_EOF )
          *s->ptr ++ = STD_CTRLZ_CODE;
        skip = 2;
      }
      dptr += skip;
    }
  } 
  if( s->ptr > orig )
  {
    s->res = ELUA_NET_ERR_OK;
    s->len = maxsize - ( s->ptr - orig );
    uip_stop();
    s->state = ELUA_UIP_STATE_IDLE;
  }
}

// Utility function for TELNET: prepend all '\n' with '\r' in buffer
// Returns actual len
// It is assumed that the buffer is "sufficiently smaller" than the UIP
// buffer (which is true for the default configuration: 128 bytes buffer
// in Newlib for stdin/stdout, more than 1024 bytes UIP buffer)
static elua_net_size elua_uip_telnet_prep_send( const char* src, elua_net_size size )
{
  elua_net_size actsize = size, i;
  char* dest = ( char* )uip_sappdata;
    
  for( i = 0; i < size; i ++ )
  {
    if( *src == '\n' )
    {
      *dest ++ = '\r';
      actsize ++;
    } 
    *dest ++ = *src ++;
  }
  return actsize;
}

#endif // #ifdef BUILD_CON_TCP

// *****************************************************************************
// eLua UIP application (used to implement the eLua TCP/IP services)

// Special handling for "accept"
volatile static u8 elua_uip_accept_request;
volatile static int elua_uip_accept_sock;
volatile static elua_net_ip elua_uip_accept_remote;

// Read data helper
static void eluah_read_data_helper( volatile struct elua_uip_state *s, elua_net_size temp )
{
  if( s->res )
    luaL_addlstring( ( luaL_Buffer* )s->ptr, ( const char* )uip_appdata, temp );        
  else
    memcpy( ( char* )s->ptr, ( const char* )uip_appdata, temp );
  s->len -= temp;
}

void elua_uip_appcall()
{
  volatile struct elua_uip_state *s;
  elua_net_size temp;
  int sockno;
  
  // If uIP is not yet configured (DHCP response not received), do nothing
  if( !elua_uip_configured )
    return;
    
  s = ( struct elua_uip_state* )&( uip_conn->appstate );
  sockno = uip_conns - uip_conn;

  if( uip_connected() )
  {
#ifdef BUILD_CON_TCP    
    if( uip_conn->lport == HTONS( ELUA_NET_TELNET_PORT ) ) // special case: telnet server
    {
      if( elua_uip_telnet_socket != -1 )
      {
        uip_close();
        return;
      }
      else
        elua_uip_telnet_socket = sockno;
    }
    else
#endif
    if( elua_uip_accept_request )
    {
      elua_uip_accept_sock = sockno;
      elua_uip_accept_remote.ipwords[ 0 ] = uip_conn->ripaddr[ 0 ];
      elua_uip_accept_remote.ipwords[ 1 ] = uip_conn->ripaddr[ 1 ];      
      elua_uip_accept_request = 0;
    }
    else if( s->state == ELUA_UIP_STATE_CONNECT )
      s->state = ELUA_UIP_STATE_IDLE;
    uip_stop();
    return;
  }

  if( s->state == ELUA_UIP_STATE_IDLE )
    return;
    
  // Do we need to read?
  if( s->state == ELUA_UIP_STATE_RECV )
  {
    // Re-enable data transfer on the socket and change state
    s->state = ELUA_UIP_STATE_RECV_2;
    uip_restart();
    return;
  }
  
  if( uip_aborted() || uip_timedout() || uip_closed() )
  {
    // Signal this error
    s->res = uip_aborted() ? ELUA_NET_ERR_ABORTED : ( uip_timedout() ? ELUA_NET_ERR_TIMEDOUT : ELUA_NET_ERR_CLOSED );
#ifdef BUILD_CON_TCP    
    if( sockno == elua_uip_telnet_socket )
      elua_uip_telnet_socket = -1;      
#endif    
    s->state = ELUA_UIP_STATE_IDLE;
    return;
  }
       
  // Handle data send  
  if( ( uip_acked() || uip_rexmit() || uip_poll() ) && ( s->state == ELUA_UIP_STATE_SEND ) )
  {
    // Special translation for TELNET: prepend all '\n' with '\r'
    // We write directly in UIP's buffer 
    if( uip_acked() )
    {
      s->len = 0;
      s->state = ELUA_UIP_STATE_IDLE;
    }
    if( s->len > 0 ) // need to (re)transmit?
    {
#ifdef BUILD_CON_TCP
      if( sockno == elua_uip_telnet_socket )
      {
        temp = elua_uip_telnet_prep_send( s->ptr, s->len );
        uip_send( uip_sappdata, temp );
      }
      else
#endif      
        uip_send( s->ptr, s->len );
    }
    return;
  }
  
  // Handle close
  if( s->state == ELUA_UIP_STATE_CLOSE )
  {
    uip_close();
    s->state = ELUA_UIP_STATE_IDLE;
    return;
  }
          
  // Handle data receive  
  if( uip_newdata() )
  {
    if( s->state == ELUA_UIP_STATE_RECV_2 )
    {
#ifdef BUILD_CON_TCP      
      if( sockno == elua_uip_telnet_socket )
      {
        elua_uip_telnet_handle_input( s );
        return;
      }
#endif   
      sockno = ELUA_NET_ERR_OK;
      // Check overflow
      if( s->len < uip_datalen() )
      {
        sockno = ELUA_NET_ERR_OVERFLOW;   
        temp = s->len;
      }
      else
        temp = uip_datalen();

      eluah_read_data_helper( s, temp );
      
      uip_stop();
      s->res = sockno;
      s->state = ELUA_UIP_STATE_IDLE;
    }
    else
      uip_stop();
  }
}

static void elua_uip_conf_static()
{
  uip_ipaddr_t ipaddr;
  uip_ipaddr( ipaddr, ELUA_CONF_IPADDR0, ELUA_CONF_IPADDR1, ELUA_CONF_IPADDR2, ELUA_CONF_IPADDR3 );
  uip_sethostaddr( ipaddr );
  uip_ipaddr( ipaddr, ELUA_CONF_NETMASK0, ELUA_CONF_NETMASK1, ELUA_CONF_NETMASK2, ELUA_CONF_NETMASK3 );
  uip_setnetmask( ipaddr ); 
  uip_ipaddr( ipaddr, ELUA_CONF_DEFGW0, ELUA_CONF_DEFGW1, ELUA_CONF_DEFGW2, ELUA_CONF_DEFGW3 );
  uip_setdraddr( ipaddr );    
  uip_ipaddr( ipaddr, ELUA_CONF_DNS0, ELUA_CONF_DNS1, ELUA_CONF_DNS2, ELUA_CONF_DNS3 );
  resolv_conf( ipaddr );  
  elua_uip_configured = 1;
}

// Init application
void elua_uip_init( const struct uip_eth_addr *paddr )
{
  // Set hardware address
  uip_setethaddr( (*paddr) );
  
  // Initialize the uIP TCP/IP stack.
  uip_init();
  uip_arp_init();  
  
#ifdef BUILD_DHCPC
  dhcpc_init( paddr->addr, sizeof( *paddr ) );
  dhcpc_request();
#else
  elua_uip_conf_static();
#endif
  
  resolv_init();
  
#ifdef BUILD_CON_TCP
  uip_listen( HTONS( ELUA_NET_TELNET_PORT ) );
#endif  
}

// *****************************************************************************
// eLua UIP UDP application (used for the DHCP client and the DNS resolver)

extern int dhcpc_socket;
extern int resolv_socket;

extern void platform_s_uart_send( unsigned, u8 );

void elua_uip_udp_appcall()
{
  volatile struct elua_uip_state *s;
  elua_net_size temp;
  int sockno;
  
  s = ( struct elua_uip_state* )&( uip_udp_conn->appstate );
  sockno = uip_udp_conn - uip_udp_conns;

  // Is this the DHCP socket?
  if( sockno == dhcpc_socket )
  {
    dhcpc_appcall();
    return;
  }

   // If uIP is not yet configured (DHCP response not received), do nothing
  if( !elua_uip_configured )
    return;

  // Is this the resolver (DNS client) socket?
  if( sockno == resolv_socket )
  {
    resolv_appcall();
    return;
  }

  // Must be an application socket, so check its state
  // Anything to send?
  if( uip_poll() && s->state == ELUA_UIP_STATE_SEND )
  {
    uip_send( s->ptr, s->len );
    s->len = 0;
    s->state = ELUA_UIP_STATE_IDLE;
  }
  else if( s->state == ELUA_UIP_STATE_CLOSE ) // handle close (trivial)
  {
    uip_udp_conn->lport = 0;
    s->state = ELUA_UIP_STATE_IDLE;
  }
  else if( uip_newdata() ) // handle data receive
  {
    if( s->recv_cb )
    {
      elua_net_ip ip;

      uip_ipaddr_copy( ( u16* )&ip, UDPBUF->srcipaddr );
      s->recv_cb( uip_appdata, uip_datalen(), ip, HTONS( uip_udp_conn->rport ) );
    }
    else if( s->state == ELUA_UIP_STATE_RECV )
    {
      sockno = ELUA_NET_ERR_OK;
      // Check overflow
      if( s->len < uip_datalen() )
      {
        sockno = ELUA_NET_ERR_OVERFLOW;   
        temp = s->len;
      }
      else
        temp = uip_datalen();

      eluah_read_data_helper( s, temp );
      
      uip_ipaddr_copy( uip_udp_conn->ripaddr, UDPBUF->srcipaddr );
      s->res = sockno;
      s->state = ELUA_UIP_STATE_IDLE;
    }
  }
}

// *****************************************************************************
// eLua TCP/IP services (from elua_net.h)

#define ELUA_UIP_IS_SOCK_OK( sock ) ( elua_uip_configured && !ELUA_UIP_IS_UDP( sock ) && sock >= 0 && sock < UIP_CONNS )
#define ELUA_UIP_IS_UDP_SOCK_OK( sock ) ( elua_uip_configured && ELUA_UIP_IS_UDP( sock ) && ELUA_UIP_FROM_UDP( sock ) >= 0 && ELUA_UIP_FROM_UDP( sock ) < UIP_UDP_CONNS )

static void elua_prep_socket_state( volatile struct elua_uip_state *pstate, void* buf, elua_net_size len, u8 res, u8 state )
{  
  pstate->ptr = ( char* )buf;
  pstate->len = len;
  pstate->res = res;
  pstate->state = state;
}

int elua_net_socket( int type )
{
  int i;
  struct uip_conn* pconn;
  struct uip_udp_conn* pudp;
  int old_status;
  volatile struct elua_uip_state *pstate = NULL;

  old_status = platform_cpu_set_global_interrupts( PLATFORM_CPU_DISABLE );
  if( type == ELUA_NET_SOCK_DGRAM )
  {
    if( ( pudp = uip_udp_new( NULL, 0 ) ) == NULL )
      i = -1;
    else
    {
      pstate = ( volatile struct elua_uip_state* )&( pudp->appstate );
      i = ELUA_UIP_TO_UDP( pudp->connidx );
    }
  }
  else
  {
    // Iterate through the list of connections, looking for a free one
    for( i = 0; i < UIP_CONNS; i ++ )
    { 
      pconn = uip_conns + i;
      if( pconn->tcpstateflags == UIP_CLOSED )
      { 
        // Found a free connection, reserve it for later use
        uip_conn_reserve( i );
        break;
      }
    }
    if( i == UIP_CONNS )
      i = -1;
    else
      pstate = ( volatile struct elua_uip_state* )&( pconn->appstate );
  }
  platform_cpu_set_global_interrupts( old_status );
  if( pstate )
  {
    memset( ( void* )pstate, 0, sizeof( *pstate ) );
    pstate->state = ELUA_UIP_STATE_IDLE;
  }
  return i;
}

// Send data - internal helper (also works for 'sendto')
static elua_net_size elua_net_send_internal( int s, const void* buf, elua_net_size len, elua_net_ip remoteip, u16 remoteport, int is_sendto )
{
  volatile struct elua_uip_state *pstate;
  elua_net_size tosend, sentbytes, totsent = 0;
  struct uip_udp_conn *pudp;
 
  if( len == 0 )
    return 0;
  if( is_sendto )
  {
    if( !ELUA_UIP_IS_UDP_SOCK_OK( s ) )
      return 0;
    s = ELUA_UIP_FROM_UDP( s );
    pstate = ( volatile struct elua_uip_state* )&( uip_udp_conns[ s ].appstate );
    pudp = uip_udp_conns + s;
    pudp->ripaddr[ 0 ] = remoteip.ipwords[ 0 ];
    pudp->ripaddr[ 1 ] = remoteip.ipwords[ 1 ];
    pudp->rport = HTONS( remoteport ); 
  }
  else
  {
    if( !ELUA_UIP_IS_SOCK_OK( s ) || !uip_conn_active( s ) )
      return 0;
    pstate = ( volatile struct elua_uip_state* )&( uip_conns[ s ].appstate );
  }
  // Send data in 'sendlimit' chunks
  while( len )
  {
    tosend = UMIN( is_sendto ? UIP_APPDATA_SIZE : uip_conns[ s ].mss, len );
    elua_prep_socket_state( pstate, ( void* )buf, tosend, ELUA_NET_ERR_OK, ELUA_UIP_STATE_SEND );
    platform_eth_force_interrupt();
    while( pstate->state != ELUA_UIP_STATE_IDLE && pstate->state != ELUA_UIP_STATE_RETRY );
    if( pstate->state == ELUA_UIP_STATE_RETRY ) // resend the exact same packet again
      continue;
    sentbytes = tosend - pstate->len;
    totsent += sentbytes;
    if( sentbytes < tosend || pstate->res != ELUA_NET_ERR_OK )
      break;
    len -= sentbytes;
    buf = ( u8* )buf + sentbytes;
  }
  return totsent;
}

elua_net_size elua_net_send( int s, const void* buf, elua_net_size len )
{
  elua_net_ip dummy;

  dummy.ipaddr = 0;
  return elua_net_send_internal( s, buf, len, dummy, 0, 0 );
}

// Internal "read" function (also works for 'recvfrom' when 'p_remote_ip' and 'p_remote_port' are not NULL)
static elua_net_size elua_net_recv_internal( int s, void* buf, elua_net_size maxsize, unsigned timer_id, s32 to_us, int with_buffer, elua_net_ip *p_remote_ip, u16 *p_remote_port, int is_recvfrom )
{
  volatile struct elua_uip_state *pstate;
  u32 tmrstart = 0;
  int old_status;
  elua_net_size readsize, readbytes, totread = 0;
 
  if( maxsize == 0 )
    return 0;
  if( is_recvfrom )
  {
    if( !ELUA_UIP_IS_UDP_SOCK_OK( s ) )
      return 0;
    s = ELUA_UIP_FROM_UDP( s );
    pstate = ( volatile struct elua_uip_state* )&( uip_udp_conns[ s ].appstate );
  }
  else
  {
    if( !ELUA_UIP_IS_SOCK_OK( s ) || !uip_conn_active( s ) )
      return 0;
    pstate = ( volatile struct elua_uip_state* )&( uip_conns[ s ].appstate );
  }
  // Read data in packets of maximum 'readlimit' bytes
  while( maxsize )
  {
    readsize = UMIN( is_recvfrom ? UIP_APPDATA_SIZE : uip_conns[ s ].mss, maxsize );
    printf( "Reading %d bytes: start\n", readsize );
    elua_prep_socket_state( pstate, buf, readsize, with_buffer, ELUA_UIP_STATE_RECV );
    if( to_us > 0 )
      tmrstart = platform_timer_op( timer_id, PLATFORM_TIMER_OP_START, 0 );
    while( 1 )
    {
      if( pstate->state == ELUA_UIP_STATE_IDLE )
        break;
      if( to_us == 0 || ( to_us > 0 && platform_timer_get_diff_us( timer_id, tmrstart, platform_timer_op( timer_id, PLATFORM_TIMER_OP_READ, 0 ) ) >= to_us ) )
      {
        old_status = platform_cpu_set_global_interrupts( PLATFORM_CPU_DISABLE );
        if( pstate->state != ELUA_UIP_STATE_IDLE )
        { 
          pstate->res = ELUA_NET_ERR_TIMEDOUT;
          pstate->state = ELUA_UIP_STATE_IDLE;
        }
        platform_cpu_set_global_interrupts( old_status );
        break;
      }
    }
    readbytes = readsize - pstate->len;
    printf( "Requested %d bytes, got %d bytes\n", readsize, readbytes );
    totread += readbytes;
    if( readbytes < readsize || pstate->res != ELUA_NET_ERR_OK )
      break;
    maxsize -= readbytes;
    if( !with_buffer )
      buf = ( u8* )buf + readbytes;
  }
  printf( "Out of read\n" );
  if( is_recvfrom )
  {
    *p_remote_port = HTONS( uip_udp_conns[ s ].rport );
    p_remote_ip->ipwords[ 0 ] = uip_udp_conns[ s ].ripaddr[ 0 ];
    p_remote_ip->ipwords[ 1 ] = uip_udp_conns[ s ].ripaddr[ 1 ];
  }
  return totread;
}

// Receive data in buf, upto "maxsize" bytes, or upto the 'readto' character if it's not -1
elua_net_size elua_net_recv( int s, void* buf, elua_net_size maxsize, unsigned timer_id, s32 to_us )
{
  return elua_net_recv_internal( s, buf, maxsize, timer_id, to_us, 0, NULL, NULL, 0 );
}

// Same thing, but with a Lua buffer as argument
elua_net_size elua_net_recvbuf( int s, luaL_Buffer* buf, elua_net_size maxsize, unsigned timer_id, s32 to_us )
{
  return elua_net_recv_internal( s, buf, maxsize, timer_id, to_us, 1, NULL, NULL, 0 );
}

// Return the socket associated with the "telnet" application (or -1 if it does
// not exist). The socket only exists if a client connected to the board.
int elua_net_get_telnet_socket()
{
  int res = -1;
  
#ifdef BUILD_CON_TCP  
  if( elua_uip_telnet_socket != -1 )
    if( uip_conn_active( elua_uip_telnet_socket ) )
      res = elua_uip_telnet_socket;
#endif      
  return res;
}

// Close socket
int elua_net_close( int s )
{
  volatile struct elua_uip_state *pstate;

  if( ELUA_UIP_IS_UDP( s ) )
  {
    if( !ELUA_UIP_IS_UDP_SOCK_OK( s ) )
      return -1;
    s = ELUA_UIP_FROM_UDP( s );
    pstate = ( volatile struct elua_uip_state* )&( uip_udp_conns[ s ].appstate );
  }
  else
  {
    if( !ELUA_UIP_IS_SOCK_OK( s ) || !uip_conn_active( s ) )
      return -1;
    pstate = ( volatile struct elua_uip_state* )&( uip_conn[ s ].appstate );
  }
  elua_prep_socket_state( pstate, NULL, 0, ELUA_NET_ERR_OK, ELUA_UIP_STATE_CLOSE );
  platform_eth_force_interrupt();
  while( pstate->state != ELUA_UIP_STATE_IDLE );
  return pstate->res == ELUA_NET_ERR_OK ? 0 : -1;
}

// Get last error on specific socket
int elua_net_get_last_err( int s )
{
  volatile struct elua_uip_state *pstate;

  if( ELUA_UIP_IS_UDP( s ) )
  {
    if( !ELUA_UIP_IS_UDP_SOCK_OK( s ) )
      return -1;
    s = ELUA_UIP_FROM_UDP( s );
    pstate = ( volatile struct elua_uip_state* )&( uip_udp_conns[ s ].appstate );
  }
  else
  {
    if( !ELUA_UIP_IS_SOCK_OK( s ) )
      return -1;
    pstate = ( volatile struct elua_uip_state* )&( uip_conn[ s ].appstate );
  }
  return pstate->res;
}

// Sets the receive callback for the socket
void elua_net_set_recv_callback( int s, p_elua_net_recv_cb callback )
{
  volatile struct elua_uip_state *pstate;

  if( ELUA_UIP_IS_UDP( s ) )
  {
    if( !ELUA_UIP_IS_UDP_SOCK_OK( s ) )
      return;
    s = ELUA_UIP_FROM_UDP( s );
    pstate = ( volatile struct elua_uip_state* )&( uip_udp_conns[ s ].appstate );
  }
  else
  {
    if( !ELUA_UIP_IS_SOCK_OK( s ) )
      return;
    pstate = ( volatile struct elua_uip_state* )&( uip_conn[ s ].appstate );
  }
  pstate->recv_cb = callback;
}

// Accept a connection on the given port, return its socket id (and the IP of the remote host by side effect)
int elua_net_accept( u16 port, unsigned timer_id, s32 to_us, elua_net_ip* pfrom )
{
  u32 tmrstart = 0;
  int old_status;
  
  if( !elua_uip_configured )
    return -1;
#ifdef BUILD_CON_TCP
  if( port == ELUA_NET_TELNET_PORT )
    return -1;
#endif  
  old_status = platform_cpu_set_global_interrupts( PLATFORM_CPU_DISABLE );
  uip_unlisten( htons( port ) );
  uip_listen( htons( port ) );
  platform_cpu_set_global_interrupts( old_status );
  elua_uip_accept_sock = -1;
  elua_uip_accept_request = 1;
  if( to_us > 0 )
    tmrstart = platform_timer_op( timer_id, PLATFORM_TIMER_OP_START, 0 );
  while( 1 )
  {
    if( elua_uip_accept_request == 0 )
      break;
    if( to_us == 0 || ( to_us > 0 && platform_timer_get_diff_us( timer_id, tmrstart, platform_timer_op( timer_id, PLATFORM_TIMER_OP_READ, 0 ) ) >= to_us ) )
    {
      elua_uip_accept_request = 0;
      break;
    }
  }  
  *pfrom = elua_uip_accept_remote;
  return elua_uip_accept_sock;
}

// Connect to a specified machine
int elua_net_connect( int s, elua_net_ip addr, u16 port )
{
  volatile struct elua_uip_state *pstate = ( volatile struct elua_uip_state* )&( uip_conns[ s ].appstate );
  uip_ipaddr_t ipaddr;
  
  if( !ELUA_UIP_IS_SOCK_OK( s ) )
    return -1;
  // The socket should have been reserved by a previous call to "elua_net_socket"
  if( !uip_conn_is_reserved( s ) )
    return -1;
  // Initiate the connect call  
  uip_ipaddr( ipaddr, addr.ipbytes[ 0 ], addr.ipbytes[ 1 ], addr.ipbytes[ 2 ], addr.ipbytes[ 3 ] );
  elua_prep_socket_state( pstate, NULL, 0, ELUA_NET_ERR_OK, ELUA_UIP_STATE_CONNECT );  
  if( uip_connect_socket( s, &ipaddr, htons( port ) ) == NULL )
    return -1;
  // And wait for it to finish
  while( pstate->state != ELUA_UIP_STATE_IDLE );
  return pstate->res == ELUA_NET_ERR_OK ? 0 : -1;
}

// Hostname lookup (resolver)
elua_net_ip elua_net_lookup( const char* hostname )
{
  elua_net_ip res;
  
  res.ipaddr = 0; 
#ifdef BUILD_DNS
  u16_t *data;
  
  if( ( data = resolv_lookup( ( char* )hostname ) ) != NULL )
  {
    // Name already saved locally
    res.ipwords[ 0 ] = data[ 0 ];
    res.ipwords[ 1 ] = data[ 1 ];
  }
  else
  {
    // Name not saved locally, must make request
    elua_resolv_req_done = 0;
    resolv_query( ( char* )hostname );
    platform_eth_force_interrupt();
    while( elua_resolv_req_done == 0 );
    res = elua_resolv_ip;
  }
#endif
  return res;  
}

// UDP send to address
unsigned elua_net_sendto( int s, const void* buf, elua_net_size len, elua_net_ip remoteip, u16 port )
{
  return elua_net_send_internal( s, buf, len, remoteip, port, 1 );
}

elua_net_size eluah_net_recvfrom_common( int s, void *buf, elua_net_size maxsize, elua_net_ip *p_remote_ip, u16 *p_remote_port, unsigned timer_id, u32 to_us, int with_buffer )
{
  elua_net_ip remote_ip;
  u16 remote_port;
  elua_net_size res = elua_net_recv_internal( s, buf, maxsize, timer_id, to_us, with_buffer, &remote_ip, &remote_port, 1 );

  if( p_remote_ip )
    p_remote_ip->ipaddr = remote_ip.ipaddr;
  if( p_remote_port )
    *p_remote_port = remote_port;
  return res;
}

elua_net_size elua_net_recvfrom( int s, void *buf, elua_net_size maxsize, elua_net_ip *p_remote_ip, u16 *p_remote_port, unsigned timer_id, s32 to_us )
{
  return eluah_net_recvfrom_common( s, buf, maxsize, p_remote_ip, p_remote_port, timer_id, to_us, 0 );
}

elua_net_size elua_net_recvfrombuf( int s, luaL_Buffer *buf, elua_net_size maxsize, elua_net_ip *p_remote_ip, u16 *p_remote_port, unsigned timer_id, s32 to_us )
{
  return eluah_net_recvfrom_common( s, buf, maxsize, p_remote_ip, p_remote_port, timer_id, to_us, 1 );
}

elua_net_ip elua_net_get_config( int what )
{
  elua_net_ip res = { 0 };
  u16 *pdns;

  if( !elua_uip_configured )
    return res;
  switch( what )
  {
    case ELUA_NET_CFG_IP:
      res.ipwords[ 0 ] = (( u16* )uip_hostaddr )[ 0 ];
      res.ipwords[ 1 ] = (( u16* )uip_hostaddr )[ 1 ];
      break;

    case ELUA_NET_CFG_NETMASK:
      res.ipwords[ 0 ] = (( u16* )uip_netmask )[ 0 ];
      res.ipwords[ 1 ] = (( u16* )uip_netmask )[ 1 ];
      break;
   
    case ELUA_NET_CFG_DNS:
      if( ( pdns = resolv_getserver() ) != NULL )
      {
        res.ipwords[ 0 ] = pdns[ 0 ];
        res.ipwords[ 1 ] = pdns[ 1 ];
      }
      break;
       
    case ELUA_NET_CFG_GW:
      res.ipwords[ 0 ] = (( u16* )uip_draddr )[ 0 ];
      res.ipwords[ 1 ] = (( u16* )uip_draddr )[ 1 ];
      break;
  }
  return res;
}

#endif // #ifdef BUILD_UIP

