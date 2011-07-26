#include "machine.h"
#include "uart.h"
#include "nrf.h"
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

static int con_putchar( char c, FILE *stream )
{
  ( void )stream;
  if( c == '\n' )
    con_putchar( '\r', stream );
  uart_putchar( c );
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM( con_putchar, NULL, _FDEV_SETUP_WRITE );

int main()
{  
  char c[] = "Incercare de test cu mata";
  unsigned res;
  u8 addr[] = NRF_CFG_SRV_PIPE0_ADDR;

  uart_init();
  sei();
  stdout = &mystdout;
  nrf_init();
  res = nrf_send_packet( addr, ( u8* )c, strlen( c ) );
  printf( "nrf_send_packet() done! Res is %d\n", res );
  while( 1 );
}

