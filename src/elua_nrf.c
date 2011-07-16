// eLua hardware layer for the nRF radio interface

#include "platform_conf.h"
#ifdef BUILD_NRF

#include "nrf_ll.h"
#include "type.h"
#include "platform.h"
#include <stdio.h>

#define NRF_IRQ_PIO_VAL       PLATFORM_PIO_ENCODE( NRF24L01_IRQ_PORT, NRF24L01_IRQ_PIN, 0 )

void nrf_ll_init( void )
{
  // GPIO configuration
  nrf_ll_ce_low();
  nrf_ll_csn_high();
  platform_pio_op( NRF24L01_CE_PORT, 1 << NRF24L01_CE_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
  platform_pio_op( NRF24L01_CSN_PORT, 1 << NRF24L01_CSN_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
  platform_pio_op( NRF24L01_IRQ_PORT, 1 << NRF24L01_IRQ_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  // UART configuration is already done in platform.c
  // Empty UART buffer
  while( platform_uart_recv( NRF24L01_UART_ID, 0, 0 ) != -1 );
}

void nrf_ll_set_ce( int state )
{
  platform_pio_op( NRF24L01_CE_PORT, 1 << NRF24L01_CE_PIN, state == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR );
}

void nrf_ll_set_csn( int state )
{
  platform_pio_op( NRF24L01_CSN_PORT, 1 << NRF24L01_CSN_PIN, state == 1 ? PLATFORM_IO_PIN_SET : PLATFORM_IO_PIN_CLEAR );
}

int nrf_ll_get_irq( void )
{
  return platform_pio_op( NRF24L01_IRQ_PORT, 1 << NRF24L01_IRQ_PIN, PLATFORM_IO_PIN_GET );
}

void nrf_ll_send_packet( const u8 *packet, u16 len )
{
  unsigned i;

  for( i = 0; i < len; i ++ )
    platform_uart_send( NRF24L01_UART_ID, packet[ i ] );
}

void nrf_ll_read_packet( u8 *packet, u16 len )
{
  int data;
  unsigned i;

  for( i = 0; i < len; i ++ )
  {
    platform_uart_send( NRF24L01_UART_ID, 0xFF );
    data = platform_uart_recv( NRF24L01_UART_ID, 0, NRF24L01_TIMEOUT );
    if( data == -1 )
      printf( "INVALID DATA RECEIVED ON NRF_LL_READ_PACKET!\n" );
    packet[ i ] = data;
  }
}

void nrf_ll_flush( u16 len )
{
  int data;
  unsigned i;

  for( i = 0; i < len; i ++ )
  {
    data = platform_uart_recv( NRF24L01_UART_ID, 0, NRF24L01_TIMEOUT );
    if( data == -1 )
      printf( "INVALID DATA RECEIVED ON NRF_LL_FLUSH!\n" );
  }
}

void nrf_ll_delay_us( u32 delay )
{
  platform_timer_delay( NRF24L01_TMR_ID, delay );
}

#endif // #ifdef BUILD_NRF

