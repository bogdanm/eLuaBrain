// nRF functions

#include "type.h"
#include "nrf.h"
#include "nrf_ll.h"

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
  // Special case: if called with len == 0 return the response to command
  // (this is used for NOP to return the status register)
  if( len > 0 )
  {
    nrf_ll_flush( 1 );
    nrf_ll_read_packet( dataptr, len );
  }
  else
    nrf_ll_read_packet( dataptr, 1 );
  nrf_ll_csn_high();
}

static void nrfh_generic_write( u8 cmd, const u8  *dataptr, u16 len )
{
  nrf_ll_csn_low();
  nrfh_ll_send_byte( cmd );
  if( len > 0 )
    nrf_ll_send_packet( dataptr, len );
  nrf_ll_csn_high();
  nrf_ll_flush( len + 1 );
}

// ****************************************************************************
// nRF commands

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
  u8 data;

  nrfh_generic_read( NRF_CMD_NOP, &data, 0 );
  return data;
}

// ****************************************************************************
// Other public functions

// Initialize nRF interface
void nrf_init()
{
  nrf_ll_init();
}

