// Asynchronous USART driver

#include "machine.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "uart.h"

// Uart variables
static volatile u8 tbuf[ UART_TBUFSIZE ];  
static volatile u8 rbuf[ UART_RBUFSIZE ];
u8 t_in;
volatile u8 t_out;
volatile u8 r_in;
u8 r_out;

// Initialize UART1
void uart_init()
{
  // Set baud rate high/low registers
  UBRRH = HI8( UART_BAUD_SELECT );
  UBRRL = LO8( UART_BAUD_SELECT ); 
  
  // Enable double speed
  UCSRA = _BV( U2X );
  
  // Ensure 8N1 mode
  UCSRC = _BV( URSEL ) | _BV( UCSZ1 ) | _BV( UCSZ0 );
  UCSRB = _BV( RXEN ) | _BV( TXEN ) | _BV( RXCIE );        
  
  // Pullup the RX line
  PORTD |= _BV( PD0 );

  t_in = t_out = 0;
  r_in = r_out = 0;
}

// Data register empty handler
ISR( USART_UDRE_vect )
{
  if( t_in != t_out )
  {
    UDR = tbuf[ t_out ];
    UART_INC_TX_BUFFER_IDX( t_out );
  }
  else 
  {
    UCSRB &= ~_BV( UDRIE );
  }
}

// Receive handler
ISR( USART_RXC_vect )
{
  u8 c;
  
  c = UDR;
  rbuf[ r_in ] = c;
  UART_INC_RX_BUFFER_IDX( r_in );
}

// Return the number of bytes in the receive buffer
u8 uart_bytes_received()
{
  return ( ( u16 )r_in + UART_RBUFSIZE - r_out ) & UART_RMASK;
}

// Read a character from the receive buffer and return the char if found,
// or -1 if the receive buffer is empty        
s16 uart_nb_getchar()
{
  u8 c;

  if( uart_bytes_received() > 0 ) 
  {
    c = rbuf[ r_out ];
    UART_INC_RX_BUFFER_IDX( r_out );
    return c;
  } 
  else 
    return -1;
}

// Put a char in the transmit buffer
void uart_putchar( u8 c )
{
  while( ( ( t_in + 1 ) & UART_TMASK ) == t_out );
  tbuf[ t_in ] = c;
  UART_INC_TX_BUFFER_IDX( t_in );
  UCSRB |= _BV( UDRIE );
}

// Returns a char (blocking version)
u8 uart_getchar()
{
  s16 c;
  
  while( ( c = uart_nb_getchar() ) == -1 );
  return c;
}

// Wait for a char at most 'to' milliseconds, then return true if the char 
// was received, or false otherwise
u8 uart_getchar_to( u8 *data, u16 to )
{
  s16 c;
  
  while( ( c = uart_nb_getchar() ) == -1 )
  {
    if( to == 0 )
      return 0;
    _delay_ms( 1.0 );
    to --;
  }
  *data = ( u8 )c;
  return 1;
}

// Empties the receive buffer
void uart_empty_rx_buffer()
{
  while( uart_nb_getchar() != -1 ) nop();
}

// Empty the transmit buffer
void uart_empty_tx_buffer()
{
  while( t_in != t_out ) nop();
}
