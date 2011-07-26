// AVR hardware layer for the nRF radio interface

#include "nrf_ll.h"
#include "type.h"
#include <stdio.h>
#include <avr/io.h>
#include "machine.h"
#include <util/delay.h>

// This is fixed
#define SPI_SS_OUT            PORTB
#define SPI_SS_IN             PINB
#define SPI_SS_DIR            DDRB
#define SPI_SS_PIN            PB2
#define SPI_MOSI_PIN          PB3
#define SPI_MISO_PIN          PB4
#define SPI_SCK_PIN           PB5

// ****************************************************************************
// Helpers 

static u8 nrfh_send_recv( data )
{
  SPDR = data;
  while( ( SPSR & _BV( SPIF ) ) == 0 );
  return SPDR;
}

// ****************************************************************************
// Public interface

void nrf_ll_init( void )
{
  nrf_ll_ce_low();
  nrf_ll_csn_high();
  DDRB |= _BV( SPI_SS_PIN ) | _BV( SPI_MOSI_PIN ) | _BV( SPI_SCK_PIN );
  NRF_PORT_DIR |= _BV( NRF_PIN_CE );
  // Enable SPI in master mode, MSB first, mode 0, Fosc/2
  SPSR = _BV( SPI2X );
  SPCR = _BV( SPE ) | _BV( MSTR );
}

void nrf_ll_set_ce( int state )
{
  if( state == 0 )
    NRF_PORT_OUT &= ( u8 )~_BV( NRF_PIN_CE );
  else
    NRF_PORT_OUT |= _BV( NRF_PIN_CE );
}

void nrf_ll_set_csn( int state )
{
  if( state == 0 )
    SPI_SS_OUT &= ( u8 )~_BV( SPI_SS_PIN );
  else
    SPI_SS_OUT |= _BV( SPI_SS_PIN );
}

int nrf_ll_get_irq( void )
{
  return ( NRF_PORT_IN & _BV( NRF_PIN_IRQ ) ) != 0;
}

void nrf_ll_send_packet( const u8 *packet, u16 len )
{
  u16 i;

  for( i = 0; i < len; i ++ )
    nrfh_send_recv( packet[ i ] );
}

void nrf_ll_read_packet( u8 *packet, u16 len )
{
  u16 i;

  for( i = 0; i < len; i ++ )
    packet[ i ] = nrfh_send_recv( 0xFF );
}

void nrf_ll_flush( u16 len )
{
  ( void )len;
}

void nrf_ll_delay_us( u32 delay )
{
  u32 i;

  for( i = 0; i < delay; i ++ )
    _delay_us( 1.0 );
}

