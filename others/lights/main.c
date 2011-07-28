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
  char c[] = "Incercare de test cu mata";
  unsigned res;
  u8 addr[] = NRF_CFG_SRV_PIPE0_ADDR;
  u32 cnt;

  leds_init();
  uart_init();
  sei();
  while( 1 )
  {
    cr = rand() % ( LED_PWM_STEPS + 1 );
    cg = rand() % ( LED_PWM_STEPS + 1 );
    cb = rand() % ( LED_PWM_STEPS + 1 );
    sync = 1;
    for( cnt = 0; cnt < 200000UL; cnt ++ ) 
      _delay_us( 1.0 );
  }
  
  stdout = &mystdout;
  nrf_init();
  res = nrf_send_packet( addr, ( u8* )c, strlen( c ) );
  printf( "nrf_send_packet() done! Res is %d\n", res );
  while( 1 );
}

