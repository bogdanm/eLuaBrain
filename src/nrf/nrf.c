// nRF functions

#include "type.h"
#include "nrf.h"
#include "nrf_ll.h"
#include "nrf_conf.h"
#include <stdio.h>

// ****************************************************************************
// Local variables and definitions

static const u8 nrf_srv_p0_addr[] = NRF_CFG_SRV_PIPE0_ADDR;
static const u8 nrf_srv_p1_addr[] = NRF_CFG_SRV_PIPE1_ADDR;

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

static void nrfh_generic_write( u8 cmd, const u8  *dataptr, u16 len )
{
  nrf_ll_csn_low();
  nrfh_ll_send_byte( cmd );
  if( len > 0 )
    nrf_ll_send_packet( dataptr, len );
  nrf_ll_flush( len + 1 );    
  nrf_ll_csn_high();
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

nrf_stat_reg_t nrf_get_status()
{
  nrf_stat_reg_t r;

  r.val = nrf_read_register_byte( NRF_REG_STATUS );
  return r;
}

// ****************************************************************************
// Higher level nRF commands

nrf_config_reg_t nrf_get_config()
{
  nrf_config_reg_t data;

  data.val = nrf_read_register_byte( NRF_REG_CONFIG );
  return data;
}

void nrf_set_config( nrf_config_reg_t conf )
{
  nrf_write_register_byte( NRF_REG_CONFIG, conf.val );
}

nrf_setup_retr_t nrf_get_setup_retr()
{
  nrf_setup_retr_t data;

  data.val = nrf_read_register_byte( NRF_REG_SETUP_RETR );
  return data;
}

void nrf_set_setup_retr( unsigned delay, unsigned count )
{
  nrf_setup_retr_t data;

  data.val = 0;
  data.fields.ard = delay;
  data.fields.arc = count;
  nrf_write_register_byte( NRF_REG_SETUP_RETR, data.val );
}

nrf_rf_setup_t nrf_get_rf_setup()
{
  nrf_rf_setup_t data;

  data.val = nrf_read_register_byte( NRF_REG_RF_SETUP );
  return data;
}

void nrf_set_rf_setup( int data_rate, int pwr, int lna )
{
  nrf_rf_setup_t data;

  data.val = 0;
  data.fields.rf_dr = data_rate;
  data.fields.rf_pwr = pwr;
  data.fields.lna_hcurr = lna;
  nrf_write_register_byte( NRF_REG_RF_SETUP, data.val );
}

nrf_fifo_status_t nrf_get_fifo_status()
{
  nrf_fifo_status_t data;

  data.val = nrf_read_register_byte( NRF_REG_FIFO_STATUS );
  return data;
}

void nrf_set_rx_addr( int pipe, const u8* paddr )
{
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

// ****************************************************************************
// Other public functions

// Initialize nRF interface
void nrf_init()
{
  u8 t[5];
  unsigned i, j;
  
  // Do low-level setup first
  nrf_ll_init();

  printf( "STAT: %d\n", nrf_read_register_byte( NRF_REG_STATUS ) );
  printf( "CONFIG: %d\n", nrf_read_register_byte( NRF_REG_CONFIG ) );
  printf( "ADDRESSES: \n");
  for( i = 0; i < 2; i ++ )
  {
    printf( "  %d -> ", i );
    nrf_get_rx_addr( i, t );
    for( j = 0; j < 5; j ++ )
      printf( "%02X%s", t[ j ], j == 4 ? "\n" : ":" );
  }

#if 1
  // Setup the actual nRF configuration now
  // Enable 'auto acknowledgement' function on all pipes
  nrf_write_register_byte( NRF_REG_EN_AA, 0x3F );
  // 5 bytes for the address field
  nrf_write_register_byte( NRF_REG_SETUP_AW, NRF_SETUP_AW_SIZE_5 );
  // Retry 1 time, start with the default timeout (0)
  nrf_write_register_byte( NRF_REG_SETUP_RETR, 0x01 );
  // Set RF channel
  nrf_write_register_byte( NRF_REG_RF_CH, NRF_CFG_RF_CHANNEL );
  // RF setup (LNA is set to 1)
  nrf_set_rf_setup( NRF_CFG_DATA_RATE, NRF_CFG_TX_POWER, 1 );
  // Setup pipe address(es)
#ifdef NRF_CFG_PROFILE_SERVER
  // Enable the first two pipes
  nrf_write_register_byte( NRF_REG_EN_RXADDR, 0x03 );
  // Set addresses
  nrf_set_rx_addr( 0, nrf_srv_p0_addr );
  nrf_set_rx_addr( 1, nrf_srv_p1_addr );
  // Set payload size
  nrf_set_payload_size( 0, NRF_PAYLOAD_SIZE );
  nrf_set_payload_size( 1, NRF_PAYLOAD_SIZE ); 
#else
  // Enable the first pipe
  nrf_write_register_byte( NRF_REG_EN_RXADDR, 0x01 );
  // Set addresses [TODO]
  // Set payload size
  nrf_set_payload_size( 0, NRF_PAYLOAD_SIZE );
#endif
  // Finally set the CONFIG register
  // Power up, transmitter, 2 bytes CRC, interrupts disabled
  nrf_write_register_byte( NRF_REG_CONFIG, 0x0E );
#endif  

  // Power up, transmitter, 2 bytes CRC, interrupts disabled
  printf( "\nAFTER\n\nSTAT: %d\n", nrf_read_register_byte( NRF_REG_STATUS ) );
  printf( "CONFIG: %d\n", nrf_read_register_byte( NRF_REG_CONFIG ) );
  printf( "ADDRESSES: \n");
  for( i = 0; i < 2; i ++ )
  {
    printf( "  %d -> ", i );
    nrf_get_rx_addr( i, t );
    for( j = 0; j < 5; j ++ )
      printf( "%02X%s", t[ j ], j == 4 ? "\n" : ":" );
  }
}

// nRF IRQ handler
void nrf_irq_handler()
{
}

