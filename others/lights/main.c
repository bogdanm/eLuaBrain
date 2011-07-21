#include "machine.h"
#include "uart.h"
#include <avr/interrupt.h>

int main()
{  
  u8 c;
  u16 i;

  uart_init();
  sei();
  while( 1 )
  {
    //c = uart_getchar();
    //uart_putchar( c );
    uart_putchar( 'A' );
    for( i = 0; i < 65535; i ++ ) nop2();
  }
  return 0;
}

