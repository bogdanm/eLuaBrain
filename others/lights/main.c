#include "machine.h"
#include "uart.h"
#include "nrf.h"
#include "ledvm.h"
#include "i2cee.h"
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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
static const u8 remote_addr[] = NRF_SRV_HW_ADDR;
// Program data
static u8 pgm_mask;
static int pgm_crt;
static u8 pgm_recv_on;
static u8 pgm_recv_slot;
static u16 pgm_recv_addr;
static u8 pgm_recv_buffer[ PGM_EE_PAGE_SIZE ];
static u8 pgm_recv_buf_idx;
static u8 pgm_cycle;
static const u8 pgm_radio_ack[] = { 0xA0, 0xC5, 0xDE, 0x17 };
// Delay data
static volatile u32 delay_cnt;
// Buttons
static u8 btn_debounce_mask, btn_state, btn_val_prev;
static volatile u8 btn_counters[ BTN_TOTAL ];

#define TIMER_PRESCALER                   1
#define start_timer()                     TCCR1B  = ( TCCR1B & ( u8 )~0x07 ) | TIMER_PRESCALER 
#define stop_timer()                      TCCR1B &= ~0x07

#define PGMADDR                           ( u8* )0

// Commands from remote 
#define CMD_START_WRITE                   0xFF // parameters: i, i, program number
#define CMD_END_WRITE                     0xFE // no parameters
#define CMD_RUN                           0xFD // parameters: i, i, program number
#define CMD_RAW                           0xFC // parameters: r, g, b
#define CMD_DEL                           0xFB // parameters: i, i, progam number

// ****************************************************************************
// Button handling

static void btn_init()
{
  BTN_PORT_OUT |= BTN_ALL_MASK;
  BTN_PORT_DIR &= ( u8 )~BTN_ALL_MASK;
  btn_val_prev = BTN_PORT_IN;
}

// Handle button debouncing
static void btn_handler()
{
  u8 v = BTN_PORT_IN, mask, i;

  for( i = 0, mask = 1 << BTN_FIRST; i < BTN_TOTAL; i ++, mask <<= 1 )
    if( btn_debounce_mask & mask ) // already debouncing button
    {
      if( btn_counters[ i ] == 0 ) // end debounce, check state
      {
        if( ( v & mask ) == 0 )
          btn_state |= mask;
        else
          btn_state &= ( u8 )~mask;
        btn_debounce_mask &= ( u8 )~mask;
      }
    }
    else if( ( ( v & mask ) == 0 ) && ( btn_val_prev & mask ) ) // button pressed, start debouncing
    {
      btn_debounce_mask |= mask;
      btn_counters[ i ] = 0xFF;
    }
  btn_val_prev = v;
}

static int btn_is_pressed( u8 idx )
{
  u8 mask = 1 << idx, v;

  if( btn_debounce_mask & mask )
    return 0;
  v = btn_state & mask;
  btn_state &= ( u8 )~mask;
  return v ? 1 : 0;
}

// ****************************************************************************
// Repeat LED indicator

#define ind_on()                          IND_PORT_OUT |= _BV( IND_PIN )
#define ind_off()                         IND_PORT_OUT &= ( u8 )~_BV( IND_PIN )

static void ind_init()
{
  ind_off();
  IND_PORT_DIR |= _BV( IND_PIN );
}

static void ind_set( int value )
{
  pgm_cycle = value;
  if( value )
    ind_on();
  else
    ind_off();
}

// ****************************************************************************
// LED handling

static void leds_init()
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
  if( delay_cnt > 0 )
    delay_cnt --;
  // Unroll button debouncing loop for speed
  if( btn_counters[ 0 ] > 0 )
    btn_counters[ 0 ] --;
  if( btn_counters[ 1 ] > 0 )
    btn_counters[ 1 ] --;
  if( btn_counters[ 2 ] > 0 )
    btn_counters[ 2 ] --;
  // Time to sync  ?
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
// Program handling

static int pgm_get_first_index()
{
  u8 i;

  for( i = 0; i < PGM_MAX; i ++ )
    if( pgm_mask & ( 1 << i ) )
      return i;
  return -1;
}

static int pgm_get_next_index( int start, int direction )
{
  u8 i = 0;

  while( i < PGM_MAX - 1 )
  {
    start = start + direction;
    if( start < 0 )
      start = PGM_MAX - 1;
    if( start > PGM_MAX - 1 )
      start = 0;
    if( pgm_mask & ( 1 << start ) )
      return start;
    i ++;
  }
  return -1;
}

static void pgm_init()
{
  // Check internal EEPROM data 
  if( eeprom_read_byte( PGMADDR ) == 0xFF ) // initial EEPROM is blank
  {
    eeprom_write_byte( PGMADDR, 0x00 );
    eeprom_busy_wait();
  }
  pgm_mask = eeprom_read_byte( PGMADDR );
  pgm_crt = pgm_get_first_index();
}

static void pgm_recv_to_flash()
{
  i2cee_write_block( pgm_recv_slot * PGM_MAX_BYTES + pgm_recv_addr, pgm_recv_buffer, pgm_recv_buf_idx );
  while( !i2cee_is_write_finished() );
  pgm_recv_addr += pgm_recv_buf_idx;
  pgm_recv_buf_idx = 0;
  nrf_set_mode( NRF_MODE_TX );
  nrf_send_packet( remote_addr, pgm_radio_ack, 4 );
  nrf_set_mode( NRF_MODE_RX );
}

void pgm_set_program_state( u8 slot, u8 state )
{
  u8 newstate = state ? ( pgm_mask | ( 1 << slot ) ) : ( pgm_mask & ( u8 )~( 1 << slot ) );

  if( newstate == pgm_mask )
    return;
  pgm_mask = newstate;
  eeprom_write_byte( PGMADDR, pgm_mask );
  eeprom_busy_wait();
}

// ****************************************************************************
// Helpers and miscellaneous functions

#if 0
static int con_putchar( char c, FILE *stream )
{
  ( void )stream;
  if( c == '\n' )
    con_putchar( '\r', stream );
  uart_putchar( c );
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM( con_putchar, NULL, _FDEV_SETUP_WRITE );
#endif

void ledvm_ll_setrgb( s8 r, s8 g, s8 b )
{
  cr = r;
  cg = g;
  cb = b;
  sync = 1;
  while( sync );
}

void ledvm_ll_delayms_start( u32 ms )
{
  u32 temp = ( ms << 4 ) / 5; // fixed formula, CHANGE IF PWM FREQUENCY CHANGES! 

  stop_timer();
  delay_cnt = temp; 
  start_timer();
}

int ledvm_ll_delay_elapsed()
{
  return delay_cnt == 0;
}

int ledvm_ll_getinst( u16 pc, u8 *pinst )
{
  if( pgm_crt != -1 )
  {
    i2cee_read_block( pgm_crt * PGM_MAX_BYTES + pc * 4, pinst, 4 );
    return 1;
  }
  else
    return 0;
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
  int res;

  leds_init();
  btn_init();
  ind_init();
  uart_init();
  sei();
  ledvm_init();
  i2cee_init();
  pgm_init();
  //stdout = &mystdout;
  nrf_init();
  nrf_set_own_addr( lights_addr );
  nrf_set_mode( NRF_MODE_RX );

  while( 1 )
  {
    // Something to execute?
    if( pgm_crt != -1 )
    {
      res = ledvm_run();
      if( res != LEDVM_ERR_OK )
      {
        if( res == LEDVM_ERR_FINISHED ) // program finished, look for the next one
        {
          if( pgm_cycle )
          {
            res = pgm_get_next_index( pgm_crt, 1 );
            if( res != -1 ) // if found use the other program, otherwise don't change it
              pgm_crt = res;
          }
          ledvm_init();
        }
        else
        {
          ledvm_ll_setrgb( 0, 0, 0 );
          pgm_crt = -1;
        }
      }
    }
      
    // New data via radio?
    if( nrf_get_packet( data, 4, NULL ) == 4 )
    {
      // Got data, interpret it
      if( data[ 0 ] == CMD_START_WRITE )
      {
        pgm_recv_on = 1;
        pgm_recv_slot = data[ 3 ];
        pgm_recv_addr = 0;
        pgm_recv_buf_idx = 0;
        if( pgm_recv_slot == pgm_crt )
          pgm_crt = -1;
      }
      else if( data[ 0 ] == CMD_END_WRITE )
      {
        pgm_recv_on = 0;
        pgm_recv_to_flash();
        pgm_set_program_state( pgm_recv_slot, 1 );
        if( pgm_crt == -1 )
          pgm_crt = pgm_get_first_index();
      }
      else if( data[ 0 ] == CMD_RUN )
      {
        if( pgm_mask & ( 1 << data[ 3 ] ) )
        {
          pgm_crt = data[ 3 ];
          ledvm_init();
        }
      }
      else if( data[ 0 ] == CMD_RAW )
      {
        pgm_crt = -1;
        ledvm_ll_setrgb( data[ 1 ], data[ 2 ], data[ 3 ] );
      }
      else if( data[ 0 ] == CMD_DEL ) 
      {
        pgm_set_program_state( data[ 3 ], 0 );
        if( pgm_crt == data[ 3 ] )
        {
          pgm_crt = pgm_get_first_index();
          ledvm_init();
        }
      }
      else if( pgm_recv_on ) // instruction
      {
        memcpy( pgm_recv_buffer + pgm_recv_buf_idx, data, 4 );
        pgm_recv_buf_idx += 4;
        if( pgm_recv_buf_idx == PGM_EE_PAGE_SIZE )
          pgm_recv_to_flash();
      }
    }

    // Handle buttons
    if( pgm_crt != -1 )
    {
      btn_handler();
      if( btn_is_pressed( BTN_NEXT_PIN ) )
      {
        res = pgm_get_next_index( pgm_crt, 1 );
        if( res != -1 )
          pgm_crt = res;
        ledvm_init();
      }
      else if( btn_is_pressed( BTN_PREV_PIN ) )
      {
        res = pgm_get_next_index( pgm_crt, -1 );
        if( res != -1 )
          pgm_crt = res;
        ledvm_init();
      }
      else if( btn_is_pressed( BTN_REPEAT_PIN ) )
        ind_set( 1 - pgm_cycle );
    }
  }  
}

