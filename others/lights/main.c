#include "machine.h"
#include "uart.h"
#include "nrf.h"
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

// ****************************************************************************
// Local variables and definitions

static u8 pwm;
static s8 vr, vg, vb;
static volatile s8 cr, cg, cb, sync;
static u8 temp;
static const u8 lights_addr[] = NRF_HW_ADDR;

#define TIMER_PRESCALER                   1
#define start_timer()                     TCCR1B  = ( TCCR1B & ( u8 )~0x07 ) | TIMER_PRESCALER 
#define stop_timer()                      TCCR1B &= ~0x07

// ****************************************************************************
// LED handling

void leds_init()
{
  LED_PORT_DIR |= _BV( LED_R_PIN ) | _BV( LED_G_PIN ) | _BV( LED_B_PIN );
  cr = cg = cb = 0;
  stop_timer();
  TCNT1 = 0;
  // CTC mode with TOP in OCR1A
  OCR1A = ( F_CPU / ( LED_BASE_FREQ_HZ * LED_PWM_STEPS ) ) - 1; 
  TCCR1A = 0;
  TCCR1B |= _BV( WGM12 );
  TIMSK |= _BV( OCIE1A );
  start_timer();
}

// Interrupt handler
ISR( TIMER1_COMPA_vect )
{
  temp = 0;
  if( pwm == 0 && sync )
  {
    vr = cr;
    vg = cg;
    vb = cb;
    sync = 0;
  }
  pwm = ( pwm + 1 ) & ( LED_PWM_STEPS - 1 );
  if( vr > pwm )
    temp |= _BV( LED_R_PIN );
  if( vg > pwm )
    temp |= _BV( LED_G_PIN );
  if( vb > pwm )
    temp |= _BV( LED_B_PIN );
  LED_PORT_OUT = ( LED_PORT_OUT & ( u8 )~LED_MASK ) | temp;
}

// ****************************************************************************
// Helpers and miscellaneous functions

static int con_putchar( char c, FILE *stream )
{
  ( void )stream;
  if( c == '\n' )
    con_putchar( '\r', stream );
  uart_putchar( c );
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM( con_putchar, NULL, _FDEV_SETUP_WRITE );

// ****************************************************************************
// Entry point

int main()
{  
  u8 data[ 4 ], len;

  leds_init();
  uart_init();
  sei();
  stdout = &mystdout;
  nrf_init();
  nrf_set_rx_addr( 0, lights_addr );
  nrf_set_mode( NRF_MODE_RX );
  while( 1 )
  {
    len = nrf_get_packet( data, 4, NULL );
    if( len != 4 )
      continue;
    if( data[ 0 ] != 1 )
      continue;
    printf( "Got data!\n" );
    cr = data[ 1 ];
    cg = data[ 2 ];
    cb = data[ 3 ];
    sync = 1;
  }
}

