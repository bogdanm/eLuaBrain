// PS/2 keyboard decoder

#include "ps2.h"
#include "platform.h"
#include "elua_int.h"
#include "type.h"
#include <stdio.h>

#include "platform_conf.h"
#ifdef BUILD_PS2

// ****************************************************************************
// Internal variables and macros

#define PS2_DATA_PIN_RESNUM   PLATFORM_IO_ENCODE( PS2_DATA_PORT, PS2_DATA_PIN, PLATFORM_IO_ENC_PIN )
#define PS2_CLOCK_PIN_RESNUM  PLATFORM_IO_ENCODE( PS2_CLOCK_PORT, PS2_CLOCK_PIN, PLATFORM_IO_ENC_PIN )      
#define PS2_QUEUE_LOG_SIZE    8
#define PS2_QUEUE_MASK        ( ( 1 << PS2_QUEUE_LOG_SIZE ) - 1 )

static u8 ps2_modifiers;
static u8 ps2_bitcnt;
static u8 ps2_data;
static u8 ps2_sending, ps2_data_to_send, ps2_send_one_bits;
static volatile int ps2_wait_ack;
static volatile u8 ps2_r_idx, ps2_w_idx;
static u8 ps2_queue[ 1 << PS2_QUEUE_LOG_SIZE ];

// ****************************************************************************
// Helpers and internal functions

// Helpers: set clock/data line high/low
static void ps2h_clock_low()
{
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
}

static void ps2h_clock_high()
{
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_INPUT );
}

static void ps2h_data_low()
{
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
}

static void ps2h_data_high()
{
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_INPUT );
}

// L->H clock line interrupt handler (for sending data)
static void ps2_lh_clk_int_handler( elua_int_resnum resnum )
{
  if( resnum != PS2_CLOCK_PIN_RESNUM )
    return;
  ps2_bitcnt ++;
  if( ps2_bitcnt < 9 ) // data bit
  {
    if( ps2_data_to_send & 1 )
    {
      ps2_send_one_bits ++;
      ps2h_data_high();
    }
    else
      ps2h_data_low();
    ps2_data_to_send >>= 1;
  }
  else if( ps2_bitcnt == 9 ) // parity bit (odd parity)
  {
    if( ps2_send_one_bits & 1 )
      ps2h_data_low();
    else
      ps2h_data_high();
  }
  else if( ps2_bitcnt == 10 ) // stop bit (release data line)
    ps2h_data_high();
  else if( ps2_bitcnt == 11 )
  {
    // Done sending, need to wait ACK from the keyboard
    ps2_sending = ps2_bitcnt = ps2_data = 0;
    platform_cpu_set_interrupt( INT_GPIO_POSEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_DISABLE );
    platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_ENABLE );
  }
}

// H->L clock line interrupt handler
static void ps2_hl_clk_int_handler( elua_int_resnum resnum )
{
  if( resnum != PS2_CLOCK_PIN_RESNUM )
    return;
  if( ps2_sending ) // send command to keyboard
  {
    ps2_send_one_bits = 0;
    // We got the first falling edge, after this the keyboard wil start to read data
    // Since the data read by the keyboard is sampled on the falling edge of the clock, 
    // we'll set it on the rising edge
    platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_DISABLE );
    platform_cpu_set_interrupt( INT_GPIO_POSEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_ENABLE );
  }
  else // receive data from keyboard
  {
    ps2_bitcnt ++;
    if( ps2_bitcnt != 1 && ps2_bitcnt < 10 ) // start/stop/parity bits, ignore
      ps2_data |= platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_GET ) << ( ps2_bitcnt - 2 );
    else if( ps2_bitcnt == 11 ) // done receiving
    {
      printf( "c=%02X ", ( unsigned )ps2_data );
      if( ps2_wait_ack == -1 )
        ps2_wait_ack = ps2_data;
      else
      {
        ps2_queue[ ps2_w_idx ] = ps2_data;
        ps2_w_idx = ( ps2_w_idx + 1 ) & PS2_QUEUE_MASK;
      }
      ps2_data = ps2_bitcnt = 0;
    }
  }
}

// Data send function, return the ACK from the keyboard
static u8 ps2_send( u8 data, int needs_ack )
{
  ps2h_clock_low();
  platform_timer_delay( PS2_TIMER_ID, 1000 );  
  ps2h_data_low();
  ps2_data_to_send = data;
  ps2_sending = 1;
  ps2_bitcnt = 0;
  if( needs_ack )
    ps2_wait_ack = -1;
  ps2h_clock_high();
  if( needs_ack )
  {
    while( ps2_wait_ack == -1 );
    printf( "%02X\n", ( unsigned )ps2_wait_ack );
    return ( u8 )ps2_wait_ack;
  }
  else
    return 1;
}

// ****************************************************************************
// Public interface

void ps2_init()
{
  // Setup pins (inputs with external pullups)
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  // Enable interrupt on clock line 
  elua_int_set_c_handler( INT_GPIO_NEGEDGE, ps2_hl_clk_int_handler ); 
  elua_int_set_c_handler( INT_GPIO_POSEDGE, ps2_lh_clk_int_handler );
  platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_ENABLE );
  // LED test
  ps2_send( 0xED, 1 );
  ps2_send( 0x03, 1 );
  // Typematic rate
  ps2_send( 0xF3, 1 );
  ps2_send( 0x00, 1 );
}

#else // #ifdef BUILD_PS2

void ps2_init()
{
}

#endif

