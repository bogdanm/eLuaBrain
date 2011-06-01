// PS/2 keyboard decoder

#include "ps2.h"
#include "platform.h"
#include "elua_int.h"

#include "platform_conf.h"
#ifdef BUILD_PS2

// Clock line interrupt handler
static void ps2_clk_int_handler( elua_int_resnum resnum )
{
  //platform_uart_send( CON_UART_ID, '!' );
//  printf( "r=%08X d=%08X c=%08X\n", ( unsigned )resnum, ( unsigned )PS2_DATA_PIN, ( unsigned )PS2_CLOCK_PIN );
  if( resnum != PS2_CLOCK_PIN )
    return;
  platform_uart_send( CON_UART_ID, '#' );
}

void ps2_init()
{
  // Enable interrupt on clock line 
  elua_int_set_c_handler( INT_GPIO_NEGEDGE, ps2_clk_int_handler ); 
  platform_cpu_set_interrupt( INT_GPIO_NEGEDGE, PS2_CLOCK_PIN, PLATFORM_CPU_ENABLE );
}

#else // #ifdef BUILD_PS2

void ps2_init()
{
}

#endif

