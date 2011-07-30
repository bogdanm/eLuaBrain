// nRF functions

#include "type.h"
#include "nrf.h"
#include "nrf_ll.h"
#include "nrf_conf.h"
#include <stdio.h>
#include <string.h>

// ****************************************************************************
// Local variables and definitions

#define NRF_SEND_RETRIES      8

static u8 nrf_p0_addr[ 5 ] = { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };
static u8 nrf_crt_mode = 0xFF;
static u32 nrf_lfsr = 1;

#ifdef NRF_DEBUG
#define nrfprint              printf
#else
void nrfprint( const char *fmt, ... )
{
}
#endif

#define NRFMIN( x, y )        ( ( x ) < ( y ) ? ( x ) : ( y ) )

// ****************************************************************************
// Helpers

static void nrfh_ll_send_byte( u8 data )
{
  nrf_ll_send_packet( &data, 1 );
}

static void nrfh_generic_read( u8 cmd, u8 *dataptr, u16 len )
{
  nrf_ll_csn_low();
  nrfh_ll_send_byte( cmd );
  nrf_ll_flush( 1 );
  nrf_ll_read_packet( dataptr, len );
  nrf_ll_csn_high();
}

static u8 nrfh_generic_read_byte( u8 cmd )
{
  u8 data;

  nrfh_generic_read( cmd, &data, 1 );
  return data;
}

static void nrfh_generic_write( u8 cmd, const u8  *dataptr, u16 len )
{
  nrf_ll_csn_low();
  nrfh_ll_send_byte( cmd );
  if( len > 0 )
    nrf_ll_send_packet( dataptr, len );
  nrf_ll_flush( len + 1 );    
  nrf_ll_csn_high();
}

static void nrfh_generic_write_byte( u8 cmd, u8 data )
{
  nrfh_generic_write( cmd, &data, 1 );
}

// LFSR based pseudo RNG from http://en.wikipedia.org/wiki/Linear_feedback_shift_register
static u32 nrfh_rand()
{
  nrf_lfsr = ( nrf_lfsr >> 1 ) ^ ( -( nrf_lfsr & 1u ) & 0xD0000001u ); 
  return nrf_lfsr;
}

// ****************************************************************************
// Basic nRF commands

void nrf_read_register( u8 address, u8 *dataptr, u16 len )
{
  nrfh_generic_read( NRF_CMD_R_REGISTER | address, dataptr, len );
}

u8 nrf_read_register_byte( u8 address )
{
  u8 data;

  nrf_read_register( address, &data, 1 );
  return data;
}

void nrf_write_register( u8 address, const u8 *dataptr, u16 len )
{
  nrfh_generic_write( NRF_CMD_W_REGISTER | address, dataptr, len );
}

void nrf_write_register_byte( u8 address, u8 data )
{
  nrf_write_register( address, &data, 1 );
}

void nrf_get_rx_payload( u8 *dataptr, u16 len )
{
  nrfh_generic_read( NRF_CMD_R_RX_PAYLOAD, dataptr, len );
}

void nrf_write_tx_payload( const u8 *dataptr, u16 len )
{
  nrfh_generic_write( NRF_CMD_W_TX_PAYLOAD, dataptr, len );
}

void nrf_flush_rx()
{
  nrfh_generic_write( NRF_CMD_FLUSH_RX, NULL, 0 );
}

void nrf_flush_tx()
{
  nrfh_generic_write( NRF_CMD_FLUSH_TX, NULL, 0 );
}

u8 nrf_get_status()
{
  return nrf_read_register_byte( NRF_REG_STATUS );
}

void nrf_activate()
{
  nrfh_generic_write_byte( NRF_CMD_ACTIVATE, 0x73 );
}

int nrf_get_payload_size()
{
  return nrfh_generic_read_byte( NRF_CMD_R_RX_PL_WID );
}

void nrf_write_ack_payload( const u8 *dataptr, u16 len )
{
  nrfh_generic_write( NRF_CMD_W_ACK_PAYLOAD, dataptr, len );
}

// ****************************************************************************
// Higher level nRF commands

void nrf_set_setup_retr( unsigned delay, unsigned count )
{
  nrf_write_register_byte( NRF_REG_SETUP_RETR, ( delay << 4 ) | count );
}

void nrf_set_rf_setup( int data_rate, int pwr, int lna )
{
  nrf_write_register_byte( NRF_REG_RF_SETUP, ( data_rate << 3 ) | ( pwr << 1 ) | lna );
}

void nrf_set_rx_addr( int pipe, const u8* paddr )
{
  unsigned i;

  if( pipe == 0 )
  {
    // This is (re)set later in nrf_set_mode
    memcpy( nrf_p0_addr, paddr, 5 );
    // And it is also used as a random seed
    nrf_lfsr = ( ( u32 )paddr[ 0 ] << 24 ) | ( ( u32 )paddr[ 1 ] << 16 ) | ( ( u32 )paddr[ 2 ] << 8 ) | paddr[ 3 ];
    for( i = 0; i < paddr[ 4 ] % 4 + 1; i ++ )
      nrfh_rand();
  }
  nrf_write_register( NRF_REG_RX_ADDR( pipe ), paddr, 5 ); 
}

void nrf_get_rx_addr( int pipe, u8 *addrbuf )
{
  nrf_read_register( NRF_REG_RX_ADDR( pipe ), addrbuf, 5 );
}

void nrf_set_tx_addr( const u8 *paddr )
{
  nrf_write_register( NRF_REG_TX_ADDR, paddr, 5 );
}

void nrf_set_payload_size( int pipe, u8 size )
{
  nrf_write_register_byte( NRF_REG_RX_PW( pipe ), size );
}

void nrf_set_mode( int mode )
{
  if( mode == nrf_crt_mode )
    return;
  nrf_write_register_byte( NRF_REG_CONFIG, ( nrf_read_register_byte( NRF_REG_CONFIG ) & 0xFE ) | mode );
  nrf_ll_set_ce( mode == NRF_MODE_RX ? 1 : 0 );
  nrf_set_rx_addr( 0, nrf_p0_addr );
  nrf_crt_mode = ( u8 )mode;
}

void nrf_clear_interrupt( u8 mask )
{
  nrf_write_register_byte( NRF_REG_STATUS, mask );
}

int nrf_has_data()
{
  return ( nrf_read_register_byte( NRF_REG_STATUS ) & NRF_INT_RX_DR_MASK ) != 0;
}

unsigned nrf_send_packet( const u8 *addr, const u8 *pdata, unsigned len )
{
  u8 stat;
  u8 retries = 0;

  if( len == 0 )
    return 0;
  nrf_set_tx_addr( addr );
  nrf_set_rx_addr( 0, addr );
  nrf_write_tx_payload( pdata, len = NRFMIN( len, NRF_PAYLOAD_SIZE ) );
  while( 1 )
  {
    nrf_ll_ce_high();
    nrf_ll_delay_us( 10 );
    nrf_ll_ce_low();
    // [TODO] change this when interrupts will be used
    while( 1 )
    {
      stat = nrf_get_status();
      if( stat & NRF_INT_TX_DS_MASK )
      {
        nrf_clear_interrupt( NRF_INT_TX_DS_MASK );
        break;
      }
      if( stat & NRF_INT_MAX_RT_MASK )
      {
        nrf_clear_interrupt( NRF_INT_MAX_RT_MASK );
        if( ++ retries == NRF_SEND_RETRIES ) // maximm retries, give up
        {
          nrf_flush_tx();
          len = 0;
        }
        else // change the timeout and the number of retries to random values
          nrf_write_register_byte( NRF_REG_SETUP_RETR, ( ( nrfh_rand() % 16 ) << 4 ) | ( nrfh_rand() % 7 + 3 ) );
        break;
      }
    }
    if( len == 0 )
      break;
    if( stat & NRF_INT_TX_DS_MASK )
      break;
  }
  return len;
}

unsigned nrf_get_packet( u8 *pdata, unsigned maxlen, int *pipeno )
{
  u8 stat;

  stat = nrf_get_status();
  if( ( stat & NRF_INT_RX_DR_MASK ) == 0 )
    return 0;
  // [TODO] this should probably be repeated while the RX FIFO still 
  // has data
  nrf_ll_ce_low();
  if( pipeno )
    *pipeno = ( stat >> 1 ) & 0x07;
  nrf_get_rx_payload( pdata, maxlen = NRFMIN( maxlen, nrf_get_payload_size() ) );
  if( ( ( nrf_get_status() >> 1 ) & 0x07 ) == NRF_RX_FIFO_EMPTY )
    nrf_clear_interrupt( NRF_INT_RX_DR_MASK );
  nrf_ll_ce_high();
  return maxlen;
}

// ****************************************************************************
// Other public functions

// Initialize nRF interface
void nrf_init()
{
  u8 t[5];
  unsigned i, j;
  
  // Do low-level setup first
  nrf_ll_init();

  nrfprint( "STAT: %d\n", nrf_read_register_byte( NRF_REG_STATUS ) );
  nrfprint( "CONFIG: %d\n", nrf_read_register_byte( NRF_REG_CONFIG ) );
  nrfprint( "ADDRESSES: \n");
  for( i = 0; i < 2; i ++ )
  {
    nrfprint( "  %d -> ", i );
    nrf_get_rx_addr( i, t );
    for( j = 0; j < 5; j ++ )
      nrfprint( "%02X%s", t[ j ], j == 4 ? "\n" : ":" );
  }

  // Setup the actual nRF configuration now
  // Put the chip in low power mode in order to access all its registers
  nrf_write_register_byte( NRF_REG_CONFIG, 0x08 );
  // Enable 'auto acknowledgement' function on all pipes
  nrf_write_register_byte( NRF_REG_EN_AA, 0x3F );
  // 5 bytes for the address field
  nrf_write_register_byte( NRF_REG_SETUP_AW, NRF_SETUP_AW_SIZE_5 );
  // Retry 1 time, start with the default timeout (0)
  // [TODO] uncomment this later
  //nrf_write_register_byte( NRF_REG_SETUP_RETR, 0x01 );
  // Set RF channel
  nrf_write_register_byte( NRF_REG_RF_CH, NRF_CFG_RF_CHANNEL );
  // RF setup (LNA is set to 1)
  nrf_set_rf_setup( NRF_CFG_DATA_RATE, NRF_CFG_TX_POWER, NRF_RF_SETUP_LNA_ON );
  // 'Activate' to enable some commands
  nrf_activate(); 
  // Enable dynamic payload length
  // [TODO] this must be modified to 0x06 to enable payload with ack
  nrf_write_register_byte( NRF_REG_FEATURE, 0x04 );
  // Enable the first pipe
  nrf_write_register_byte( NRF_REG_EN_RXADDR, 0x01 );
  // Set pipe to default address
  nrf_set_rx_addr( 0, nrf_p0_addr );
  // Set payload size (not sure if this is needed in dynamic payload mode)
  nrf_set_payload_size( 0, NRF_PAYLOAD_SIZE );
  // Enable dynamic payload
  nrf_write_register_byte( NRF_REG_DYNPD, 0x01 );
  // Finally set the CONFIG register
  // Power up, 2 bytes CRC, interrupts disabled
  // [TODO] change interrupts to enabled eventually
  nrf_write_register_byte( NRF_REG_CONFIG, 0x0E );
  // Always start in TX mode
  nrf_set_mode( NRF_MODE_TX );

  // Power up, transmitter, 2 bytes CRC, interrupts disabled
  nrfprint( "\nAFTER\n\nSTAT: %d\n", nrf_read_register_byte( NRF_REG_STATUS ) );
  nrfprint( "CONFIG: %d\n", nrf_read_register_byte( NRF_REG_CONFIG ) );
  nrfprint( "ADDRESSES: \n");
  for( i = 0; i < 2; i ++ )
  {
    nrfprint( "  %d -> ", i );
    nrf_get_rx_addr( i, t );
    for( j = 0; j < 5; j ++ )
      nrfprint( "%02X%s", t[ j ], j == 4 ? "\n" : ":" );
  }
  nrfprint( "FEATURE: %d\n", nrf_read_register_byte( NRF_REG_FEATURE ) );
}

// nRF IRQ handler
void nrf_irq_handler()
{
}

