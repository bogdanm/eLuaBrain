// PS/2 keyboard decoder

#include "ps2.h"
#include "platform.h"
#include "elua_int.h"
#include "type.h"
#include "term.h"
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#include "platform_conf.h"
#ifdef BUILD_PS2

// ****************************************************************************
// Internal variables and macros

#define PS2_DATA_PIN_RESNUM   PLATFORM_IO_ENCODE( PS2_DATA_PORT, PS2_DATA_PIN, PLATFORM_IO_ENC_PIN )
#define PS2_CLOCK_PIN_RESNUM  PLATFORM_IO_ENCODE( PS2_CLOCK_PORT, PS2_CLOCK_PIN, PLATFORM_IO_ENC_PIN )      
#define PS2_QUEUE_LOG_SIZE    8
#define PS2_QUEUE_MASK        ( ( 1 << PS2_QUEUE_LOG_SIZE ) - 1 )
#define PS2_SEND_BUFFER_SIZE  16

// Internal (non-public) flags
#define PS2_IF_EXTENDED       0x0008
#define PS2_IF_BREAK          0x0010
#define PS2_IF_EXT_CTRL       0x0020
#define PS2_IF_EXT_ALT        0x0040
#define PS2_IF_CTRL           0x0080
#define PS2_IF_ALT            0x0100
#define PS2_IF_LSHIFT         0x0200
#define PS2_IF_RSHIFT         0x0400
#define PS2_IF_CAPS           0x0800
#define ps2_set_flag( x )     ps2_flags |= ( x )
#define ps2_clear_flag( x )   ps2_flags &= ( u16 )~( x )
#define ps2_is_set( x )       ( ( ps2_flags & ( x ) ) != 0 )

// PS/2 codes
#define PS2_EXTENDED_CODE     0xE0
#define PS2_BREAK_CODE        0xF0
#define PS2_PAUSE_1ST_CODE    0xE1
#define PS2_PAUSE_NUM_CODES   8
#define PS2_CTRL_CODE         0x14
#define PS2_ALT_CODE          0x11
#define PS2_LSHIFT_CODE       0x12
#define PS2_RSHIFT_CODE       0x59
#define PS2_PRNT_EXT_CODE     0x7C
#define PS2_LAST_MAP_CODE     0x7F
#define PS2_F7_CODE           0x83
#define PS2_CAPS_CODE         0x58

// PS2/ commands
#define PS2_CMD_LED_SET       0xED
#define PS2_CMD_TYPE_RATE     0xF3
#define PS2_CMD_RESET         0xFF
#define PS2_CAPS_LED_MASK     0x04
// "FAST" I/O operations
#define ps2h_clock_low()      platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_OUTPUT )
#define ps2h_clock_high()     platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_INPUT )
#define ps2h_data_low()       platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_OUTPUT )
#define ps2h_data_high()      platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_INPUT )
#define ps2h_clock_get()      platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_GET )
#define ps2h_data_get()       platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_GET )
#define ps2h_reset()          platform_pio_op( PS2_RESET_PORT, 1 << PS2_RESET_PIN, PLATFORM_IO_PIN_DIR_OUTPUT )

static u16 ps2_flags;                   // internal and public flags
static u8 ps2_bitcnt;                   // PS/2 data bit counter
static u8 ps2_data;                     // PS/2 data read from keyboard
static volatile int ps2_wait_ack;       // sender acknowledge 
static volatile u8 ps2_r_idx, ps2_w_idx;  // queue read and write pointers
static u16 ps2_queue[ 1 << PS2_QUEUE_LOG_SIZE ]; // keyboard data queue
static u8 ps2_ignore_count;             // how many codes to ignore
// Send buffer: kept as pairs of (byte to send, num acks)
static u8 ps2_send_buffer[ PS2_SEND_BUFFER_SIZE ];
static volatile u8 ps2_cmds_to_send;
static u8 ps2_send_idx;
static volatile u8 ps2_acks_to_receive;
static elua_int_c_handler ps2_prev_handler;

// Direct scancode to ASCII mappings (0 means 'no mapping')
static const u16 ps2_direct_mapping[] = 
{
           /*    0     1     2     3     4     5     6      7      8       9     A     B     C    D     E     F */
  /* 00-0F */ 0x00,KC_F9, 0x00,KC_F5,KC_F3,KC_F1,KC_F2,KC_F12,  0x00, KC_F10,KC_F8,KC_F6,KC_F4,   9,  '`', 0x00,
  /* 10-1F */ 0x00, 0x00, 0x00, 0x00, 0x00,  'q',  '1',  0x00,  0x00,  0x00,  'z',  's',  'a',  'w',  '2', 0x00,
  /* 20-2F */ 0x00,  'c',  'x',  'd',  'e',  '4',  '3',  0x00,  0x00,   ' ',  'v',  'f',  't',  'r',  '5', 0x00,
  /* 30-3F */ 0x00,  'n',  'b',  'h',  'g',  'y',  '6',  0x00,  0x00,  0x00,  'm',  'j',  'u',  '7',  '8', 0x00,
  /* 40-4F */ 0x00,  ',',  'k',  'i',  'o',  '0',  '9',  0x00,  0x00,   '.',  '/',  'l',  ';',  'p',  '-', 0x00,
  /* 50-5F */ 0x00, 0x00, '\'', 0x00,  '[',  '=', 0x00,  0x00,  0x00,  0x00,   10,  ']', 0x00, '\\', 0x00, 0x00,
  /* 60-6F */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    8,  0x00,  0x00,   '1', 0x00,  '4',  '7', 0x00, 0x00, 0x00,
  /* 70-7F */  '0',  '.',  '2',  '5',  '6',  '8', 0x1B,  0x00,KC_F11,   '+',  '3',  '-',  '*',  '9', 0x00, 0x00
};

// SHIFT scancode to ASCII mappings (0 means 'no mapping')
static const u16 ps2_shift_mapping[] = 
{
           /*    0     1     2     3     4     5     6      7      8      9     A     B     C     D     E     F */
  /* 00-0F */ 0x00,KC_F9, 0x00,KC_F5,KC_F3,KC_F1,KC_F2,KC_F12,  0x00,KC_F10,KC_F8,KC_F6,KC_F4,    9,  '~', 0x00,
  /* 10-1F */ 0x00, 0x00, 0x00, 0x00, 0x00,  'Q',  '!',  0x00,  0x00,  0x00,  'Z',  'S',  'A',  'W',  '@', 0x00,
  /* 20-2F */ 0x00,  'C',  'X',  'D',  'E',  '$',  '#',  0x00,  0x00,   ' ',  'V',  'F',  'T',  'R',  '%', 0x00,
  /* 30-3F */ 0x00,  'N',  'B',  'H',  'G',  'Y',  '^',  0x00,  0x00,  0x00,  'M',  'J',  'U',  '&',  '*', 0x00,
  /* 40-4F */ 0x00,  '<',  'K',  'I',  'O',  ')',  '(',  0x00,  0x00,   '>',  '?',  'L',  ':',  'P',  '_', 0x00,
  /* 50-5F */ 0x00, 0x00,  '"', 0x00,  '{',  '+', 0x00,  0x00,  0x00,  0x00,   10,  '}', 0x00,  '|', 0x00, 0x00,
  /* 60-6F */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    8,  0x00,  0x00,   '1', 0x00,  '4',  '7', 0x00, 0x00, 0x00,
  /* 70-7F */  '0',  '.',  '2',  '5',  '6',  '8', 0x1B,  0x00,KC_F11,   '+',  '3',  '-',  '*',  '9', 0x00, 0x00
};

// Key to code mapping
typedef struct
{
  u16 kc;
  u16 code;
} ps2_keymap;

// Extended scan code map
static const ps2_keymap ps2_ext_mapping[] = 
{
  { 0x75, KC_UP },
  { 0x72, KC_DOWN },
  { 0x6B, KC_LEFT },
  { 0x74, KC_RIGHT },
  { 0x6C, KC_HOME },
  { 0x69, KC_END },
  { 0x7D, KC_PAGEUP },
  { 0x7A, KC_PAGEDOWN },
  { 0x71, KC_DEL },
  { 0x70, KC_INSERT },
  { 0x5A, 10 },
  { 0x4A, '/' },
};

// ****************************************************************************
// Helpers and internal functions

// Wait for a high to low transition on the clock line
static void ps2h_wait_clock_hl()
{
  while( ps2h_clock_get() == 0 ); // Wait for 1
  while( ps2h_clock_get() == 1 ); // Wait for 0
}

// Send one command to the keyboard
// Gets the command and the number of expected ACKs from ps2_send_buffer at
// the index received as argument
static void ps2h_send_cmd( int idx )
{
  unsigned i;
  int par;
  int cmd = ps2_send_buffer[ idx * 2 ];
  int nacks = ps2_send_buffer[ idx * 2 + 1 ];

  platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_DISABLE );
  ps2h_clock_low();
  platform_timer_delay( PS2_TIMER_ID, 1000 );
  ps2h_data_low();
  ps2h_clock_high();
  ps2h_wait_clock_hl();
  for( i = par = 0; i < 8; i ++, cmd >>= 1 ) 
  {
    if( cmd & 1 )
    {
      ps2h_data_high();
      par ++;
    }
    else
      ps2h_data_low();
    ps2h_wait_clock_hl();
  }
  // Send parity now
  if( par & 1 )
    ps2h_data_low();
  else
    ps2h_data_high();
  ps2h_wait_clock_hl();
  // Release data line (stop bit)
  ps2h_data_high();
  ps2h_wait_clock_hl();
  // Read and interpret ACK bit
  if( ps2h_data_get() == 1 ) // error, [TODO] what to do here?
  {
  }
  while( ps2h_clock_get() == 0 );
  // Clear pending interrupts and re-enable keyboard interrupt
  ps2_acks_to_receive = nacks;
  platform_cpu_get_interrupt_flag( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_CLEAR_FLAG );
  platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_ENABLE );
}

// Data send function
// Gets pairs of (cmd/arg, nacks) as arguments)
static void ps2_send( int ncmds, ... )
{
  va_list va;
  unsigned i;

  va_start( va, ncmds );
  // Copy all arguments to to the send buffer
  for( i = 0; i < ncmds * 2; i ++ )
    ps2_send_buffer[ i ] = va_arg( va, int );
  // Initialize the PS/2 send variables
  ps2_cmds_to_send = ncmds;
  ps2_send_idx = ps2_acks_to_receive = 0;
  // Send the first arguments "as-is", the rest wil be sent by the int handler
  va_end( va );
  ps2h_send_cmd( 0 );
}

// H->L clock line interrupt handler
static void ps2_hl_clk_int_handler( elua_int_resnum resnum )
{
  int temp;
  unsigned i;

  if( resnum != PS2_CLOCK_PIN_RESNUM )
    goto done;
  ps2_bitcnt ++;
  if( ps2_bitcnt != 1 && ps2_bitcnt < 10 ) // start/stop/parity bits, ignore
    ps2_data |= platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_GET ) << ( ps2_bitcnt - 2 );
  else if( ps2_bitcnt == 11 ) // done receiving, the code is in ps2_data now
  {
    /// Check the "send" part first
    if( ps2_acks_to_receive > 0 )
    {
      ps2_data = ps2_bitcnt = 0;
      if( -- ps2_acks_to_receive == 0 )
      {
        if( -- ps2_cmds_to_send != 0 ) // more data to send?
          ps2h_send_cmd( ++ ps2_send_idx );
      }
      goto done;
    }
    else // nothing is sending, so interpret this code
    {
      if( ps2_ignore_count > 0 ) // need to ignore this code?        
        ps2_ignore_count --;
      else
      {
        if( ps2_is_set( PS2_IF_EXTENDED ) )           // extended key management
        {
          if( ps2_is_set( PS2_IF_BREAK ) )            // code of extended key that was released
          {
            if( ps2_data == PS2_CTRL_CODE )           // CTRL released
            {
              ps2_clear_flag( PS2_IF_EXT_CTRL );
              if( !ps2_is_set( PS2_IF_CTRL )  )
                ps2_clear_flag( PS2_CTRL );
            }
            else if( ps2_data == PS2_ALT_CODE )       // ALT released
            {
              ps2_clear_flag( PS2_IF_EXT_ALT );
              if( !ps2_is_set( PS2_IF_ALT ) )
                ps2_clear_flag( PS2_ALT );
            }
            ps2_clear_flag( PS2_IF_BREAK );
            ps2_clear_flag( PS2_IF_EXTENDED );
          }
          else if( ps2_data == PS2_BREAK_CODE ) 
            ps2_set_flag( PS2_IF_BREAK );
          else
          { 
            if( ps2_data == PS2_CTRL_CODE )
              ps2_set_flag( PS2_IF_EXT_CTRL | PS2_CTRL );
            if( ps2_data == PS2_ALT_CODE )
              ps2_set_flag( PS2_IF_EXT_ALT | PS2_ALT );
            else
              // Map keycode, add to queue if needed
              for( i = 0; i < sizeof( ps2_ext_mapping ) / sizeof( ps2_keymap ); i ++ ) 
                if( ps2_ext_mapping[ i ].kc == ps2_data )
                {
                  // Check for CTRL+ATL+DEL
                  if( ps2_ext_mapping[ i ].code == KC_DEL && ps2_is_set( PS2_ALT ) && ps2_is_set( PS2_CTRL ) )
                  {
                    ps2h_reset();
                    while( 1 ); // obviously we won't reach code after this
                  }
                  ps2_queue[ ps2_w_idx ] = ( ( ps2_flags & PS2_BASEF_MASK ) << PS2_BASEF_SHIFT ) | ps2_ext_mapping[ i ].code;
                  ps2_w_idx = ( ps2_w_idx + 1 ) & PS2_QUEUE_MASK;
                  break;
                }
            ps2_clear_flag( PS2_IF_EXTENDED );
          }
        } // end of extended key management
        else if( ps2_is_set( PS2_IF_BREAK ) )       // code of key that was released
        {
          if( ps2_data == PS2_CTRL_CODE )           // CTRL released
          {
            ps2_clear_flag( PS2_IF_CTRL );
            if( !ps2_is_set( PS2_IF_EXT_CTRL ) )
              ps2_clear_flag( PS2_CTRL );
          }
          else if( ps2_data == PS2_ALT_CODE )       // ALT released
          {
            ps2_clear_flag( PS2_IF_ALT );
            if( !ps2_is_set( PS2_IF_EXT_ALT ) )
              ps2_clear_flag( PS2_ALT );
          }
          else if( ps2_data == PS2_LSHIFT_CODE )
          {
            ps2_clear_flag( PS2_IF_LSHIFT );
            if( !ps2_is_set( PS2_IF_RSHIFT ) )
              ps2_clear_flag( PS2_SHIFT );
          }
          else if( ps2_data == PS2_RSHIFT_CODE )
          {
            ps2_clear_flag( PS2_IF_RSHIFT );
            if( !ps2_is_set( PS2_IF_LSHIFT ) )
              ps2_clear_flag( PS2_SHIFT );
          }
          else if( ps2_data == PS2_CAPS_CODE )
          {
            if( ps2_is_set( PS2_IF_CAPS ) )
            {
              ps2_clear_flag( PS2_IF_CAPS );
              ps2_send( 2, PS2_CMD_LED_SET, 1, 0, 1 );
            }
            else
            {
              ps2_set_flag( PS2_IF_CAPS );
              ps2_send( 2, PS2_CMD_LED_SET, 1, PS2_CAPS_LED_MASK, 1 );
            }
          }
          ps2_clear_flag( PS2_IF_BREAK );
        }          
        else if( ps2_data == PS2_PAUSE_1ST_CODE )   // ignore everything related to this crazy key
          ps2_ignore_count = PS2_PAUSE_NUM_CODES - 1;
        else if( ps2_data == PS2_EXTENDED_CODE )    // this is an extended key
          ps2_set_flag( PS2_IF_EXTENDED );
        else if( ps2_data == PS2_BREAK_CODE )       // found a break
          ps2_set_flag( PS2_IF_BREAK );
        else if( ps2_data == PS2_CTRL_CODE )        // CTRL pressed
          ps2_set_flag( PS2_IF_CTRL | PS2_CTRL );
        else if( ps2_data == PS2_ALT_CODE )         // ALT pressed
          ps2_set_flag( PS2_IF_ALT | PS2_ALT );
        else if( ps2_data == PS2_LSHIFT_CODE )      // Left SHIFT pressed
          ps2_set_flag( PS2_IF_LSHIFT | PS2_SHIFT );
        else if( ps2_data == PS2_RSHIFT_CODE )      // Right SHIFT pressed
          ps2_set_flag( PS2_IF_RSHIFT | PS2_SHIFT );
        else
        {
          // Check if we need to enqueue a key
          if( ps2_data == PS2_F7_CODE )             // special case to keep mapping arrays small
            temp = KC_F7;
          else if( ps2_data > PS2_LAST_MAP_CODE )
            temp = 0;
          else if( ps2_is_set( PS2_SHIFT ) )
            temp = ps2_shift_mapping[ ps2_data ];
          else
            temp = ps2_direct_mapping[ ps2_data ];
          if( temp )
          {
            if( ps2_is_set( PS2_IF_CAPS ) && isalpha( temp ) )
              temp = toupper( temp );
            ps2_queue[ ps2_w_idx ] = ( ( ps2_flags & PS2_BASEF_MASK ) << PS2_BASEF_SHIFT ) | temp;
            ps2_w_idx = ( ps2_w_idx + 1 ) & PS2_QUEUE_MASK;
          }           
        }
      }
    }
    ps2_data = ps2_bitcnt = 0;
  }
done:
  if( ps2_prev_handler )
    ps2_prev_handler( resnum );
}

// Helper: read data from buffer, increment pointer
static u16 ps2h_read_data()
{
  u16 data = ps2_queue[ ps2_r_idx ];
  ps2_r_idx = ( ps2_r_idx + 1 ) & PS2_QUEUE_MASK;
  return data;
}

// ****************************************************************************
// Public interface

void ps2_init()
{
  // Setup pins (inputs with external pullups)
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  platform_pio_op( PS2_RESET_PORT, 1 << PS2_RESET_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  platform_pio_op( PS2_DATA_PORT, 1 << PS2_DATA_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( PS2_CLOCK_PORT, 1 << PS2_CLOCK_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( PS2_RESET_PORT, 1 << PS2_RESET_PIN, PLATFORM_IO_PIN_CLEAR );
  // Enable interrupt on clock line 
  ps2_prev_handler = elua_int_set_c_handler( INT_GPIO_NEGEDGE, ps2_hl_clk_int_handler ); 
  platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN_RESNUM, PLATFORM_CPU_ENABLE );
  // Reset + typematic rate
  ps2_send( 3, PS2_CMD_RESET, 2, PS2_CMD_TYPE_RATE, 1, 0x00, 1 );
}

int ps2_get_rawkey( s32 timeout )
{
  timer_data_type tmr_start, tmr_crt;

  if( timeout == 0 )
  {
    if( ps2_r_idx == ps2_w_idx )
      return -1;
  }
  else
  {
    if( timeout == PLATFORM_UART_INFINITE_TIMEOUT )
      while( ps2_r_idx == ps2_w_idx );
    else
    {
      tmr_start = platform_timer_op( PS2_TIMER_ID, PLATFORM_TIMER_OP_START, 0 );
      while( 1 )
      {
        if( ps2_r_idx != ps2_w_idx )
          break;
        tmr_crt = platform_timer_op( PS2_TIMER_ID, PLATFORM_TIMER_OP_READ, 0 );
        if( platform_timer_get_diff_us( PS2_TIMER_ID, tmr_crt, tmr_start ) >= timeout )
          return -1;
      }
    }
  }
  return ps2h_read_data();
}

// Input function for PS/2 keyboard: get data according to the timeout,
// ignore everything that is not ASCII 0-255
int ps2_std_get( s32 timeout )
{
  int data, mods;

  while( 1 )
  {
    if( ( data = ps2_get_rawkey( timeout ) ) == -1 )
      return -1;
    mods = PS2_RAW_TO_MODS( data );
    if( mods & ( PS2_CTRL | PS2_ALT ) )
      continue;
    if( ( data = PS2_RAW_TO_CODE( data ) ) < 256 )
      break;
  }
  return data;
}

int ps2_term_get( int mode )
{
  s32 to = mode == TERM_INPUT_DONT_WAIT ? 0 : PLATFORM_UART_INFINITE_TIMEOUT;
  
  return ps2_get_rawkey( to );
}

// Mapping of terminal codes
static const ps2_keymap ps2_term_mapping[] =
{
  { 'z', KC_CTRL_Z },
  { 'a', KC_CTRL_A },
  { 'e', KC_CTRL_E },
  { 'c', KC_CTRL_C },
  { 't', KC_CTRL_T },
  { 'u', KC_CTRL_U },
  { 'k', KC_CTRL_K },
  { 'y', KC_CTRL_Y },
  { 'b', KC_CTRL_B },
  { 'x', KC_CTRL_X },
  { 'v', KC_CTRL_V },
  { KC_F2, KC_CTRL_F2 },
  { KC_F4, KC_CTRL_F4 },
};

int ps2_term_translate( int code )
{
  unsigned i;
  int kcode = PS2_RAW_TO_CODE( code );
  int kmods = PS2_RAW_TO_MODS( code );
  int termcode = code & ~( PS2_SHIFT << PS2_BASEF_SHIFT );

  if( ( kmods & PS2_CTRL ) && ( isalpha( kcode ) || ( kcode >= KC_F1 && kcode <= KC_F12 ) ) )
  {
    kcode = tolower( kcode );
    termcode = -1;
    for( i = 0; i < sizeof( ps2_term_mapping ) / sizeof( ps2_keymap ); i ++ )
      if( ps2_term_mapping[ i ].kc == kcode )
      {
        termcode = ps2_term_mapping[ i ].code;
        break;
      }
  }
  else if( kcode == '\n' )
    termcode = KC_ENTER;
  else if( kcode == '\t' )
    termcode = KC_TAB;
  else if( kcode == '\b' )
    termcode = KC_BACKSPACE;
  else if( kcode == 0x1B )
    termcode = KC_ESC;
  return termcode;
}

#else // #ifdef BUILD_PS2

void ps2_init()
{
}

#endif

