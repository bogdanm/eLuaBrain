// Main system definitions

#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <avr/io.h>
#include "type.h"

// *****************************************************************************
// CPU clock (Hz)

#define F_CPU                             7372800UL

// *****************************************************************************
// UART definitions
#define UART_BAUD_RATE                    115200
// Compute baud rate (assuming that U2X = 1)
#define UART_BAUD_SELECT                  ( F_CPU / ( UART_BAUD_RATE * 8L ) - 1L )
// Buffer sizes + masks 
// Must be powers of 2, LESS than 256 (otherwise uart.c should be changed)
#define UART_RBUFSIZE                     64
#define UART_TBUFSIZE                     64
#define UART_RMASK                        ( UART_RBUFSIZE - 1 )
#define UART_TMASK                        ( UART_TBUFSIZE - 1 )
#define UART_INC_RX_BUFFER_IDX( x )       x = ( x + 1 ) & UART_RMASK
#define UART_INC_TX_BUFFER_IDX( x )       x = ( x + 1 ) & UART_TMASK

// *****************************************************************************
// nRF definitions

#define NRF_PORT_IN                       PIND 
#define NRF_PORT_OUT                      PORTD
#define NRF_PORT_DIR                      DDRD
#define NRF_PIN_CE                        PD3
#define NRF_PIN_IRQ                       PD2
#define NRF_HW_ADDR                       { 0x74, 0xA3, 0xC9, 0xEF, 0x68 }

// *****************************************************************************
// LEDs definitions
#define LED_PORT_OUT                      PORTC
#define LED_PORT_IN                       PINC
#define LED_PORT_DIR                      DDRC
#define LED_G_PIN                         PC0
#define LED_R_PIN                         PC1
#define LED_B_PIN                         PC2
#define LED_MASK                          ( _BV( LED_R_PIN ) | _BV( LED_G_PIN ) | _BV( LED_B_PIN ) )
#define LED_BASE_FREQ_HZ                  100
#define LED_PWM_STEPS                     32

// *****************************************************************************
// Other macros

#define nop()                             asm( "nop\n\t":: )
#define nop2()                            asm( "nop\n\tnop\n\t":: )
#define nop4()                            asm( "nop\n\tnop\n\tnop\n\tnop\n\t":: )
#define HI8( x )                          ( ( ( x ) >> 8 ) & 0xFF )
#define LO8( x )                          ( ( x ) & 0xFF )
#define HI16( x )                         ( ( ( x ) >> 16 ) & 0xFFFF )
#define LO16( x )                         ( ( x ) & 0xFFFF )
#define EEPROM                            __attribute__((section(".eeprom")))

#endif

