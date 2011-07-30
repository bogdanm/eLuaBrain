#include "machine.h"
#include "uart.h"
#include "nrf.h"
#include "ledvm.h"
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <stdlib.h>

// ****************************************************************************
// Local variables and definitions

static u8 pwm;
static s8 vr, vg, vb;
static volatile s8 cr, cg, cb, sync;
static u8 temp;
static const u8 lights_addr[] = NRF_HW_ADDR;
static u8 program[ 4 * 128 ];

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


void ledvm_ll_setrgb( s8 r, s8 g, s8 b )
{
  cr = r;
  cg = g;
  cb = b;
  sync = 1;
  while( sync );
}

void ledvm_ll_delayms( u32 ms )
{
  u32 i;

  for( i = 0; i < ms ; i ++ )
    _delay_ms( 1.0 );
}

int ledvm_ll_getinst( u16 pc, u8 *pinst )
{
  memcpy( pinst, program + pc * 4, 4 );
  return 1;
}

int ledvm_ll_rand()
{
  return rand();
}

// ****************************************************************************
// Entry point

int main()
{  
  u8 data[ 4 ];
  u16 idx = 0, i;

  leds_init();
  uart_init();
  sei();
  ledvm_init();
  stdout = &mystdout;
  nrf_init();
  nrf_set_rx_addr( 0, lights_addr );
  nrf_set_mode( NRF_MODE_RX );
  while( 1 )
  {
    while( 1 )
    {
      // Get program
      if( nrf_get_packet( data, 4, NULL ) != 4 )
        continue;
      for( i = 0; i < 4; i ++ ) printf( "%02X", data[ i ] );
      printf( "\n" );
      if( data[ 0 ] == 0xFF && data[ 1 ] == 0xFF && data[ 2 ] == 0xFF && data[ 3 ] == 0xFF )
        break;
      memcpy( program + idx, data, 4 );
      idx += 4;
    }
    printf( "Program uploaded, got %d instructions\n", idx >> 2 );
    // Execute program
    while( 1 )
    {
      if( ( idx = ledvm_run() ) != LEDVM_ERR_OK )
      {
        printf( "ledvm_run() returned error %d at %d!\n", idx, ledvm_get_pc() );
        ledvm_ll_setrgb( 0, 0, 0 );
        while( 1 );
      }
    }
  }
}

