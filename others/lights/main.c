#include "machine.h"
#include "uart.h"
#include <avr/interrupt.h>

int main()
{  
  u8 c;

  uart_init();
  sei();
  while( 1 )
  {
    c = uart_getchar();
    uart_putchar( c == ' ' ? c : ( c - 'a' + 13 ) % 26 + 'a' );
  }
  return 0;
}

