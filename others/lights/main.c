#include "machine.h"
#include "uart.h"
#include "nrf.h"
#include <avr/interrupt.h>
#include <stdio.h>

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
  u8 c;

  uart_init();
  sei();
  stdout = &mystdout;
  nrf_init();
  while( 1 );
}

