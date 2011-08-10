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
#define NRF_SRV_HW_ADDR                   { 0xAB, 0x31, 0xC6, 0x79, 0xFE }

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
// LED program data

#define PGM_EE_SIZE                       ( 32 * 1024L )
#define PGM_EE_PAGE_SIZE                  64
#define PGM_MAX                           8
#define PGM_MAX_INSTR                     ( PGM_EE_SIZE / ( PGM_MAX * 4 ) )
#define PGM_MAX_BYTES                     ( PGM_MAX_INSTR * 4 )

// *****************************************************************************
// Buttons

#define BTN_PORT_OUT                      PORTD
#define BTN_PORT_IN                       PIND
#define BTN_PORT_DIR                      DDRD
#define BTN_NEXT_PIN                      PD5
#define BTN_PREV_PIN                      PD6
#define BTN_REPEAT_PIN                    PD7
#define BTN_FIRST                         BTN_NEXT_PIN
#define BTN_LAST                          BTN_REPEAT_PIN
#define BTN_ALL_MASK                      ( _BV( BTN_NEXT_PIN ) | _BV( BTN_PREV_PIN ) | _BV( BTN_REPEAT_PIN ) )
#define BTN_TOTAL                         3

// *****************************************************************************
// Repeat mode indicator LED

#define IND_PORT_OUT                      PORTB
#define IND_PORT_IN                       PINB
#define IND_PORT_DIR                      DDRB
#define IND_PIN                           PB0

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

