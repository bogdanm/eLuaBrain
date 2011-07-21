// Main system definitions

#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <avr/io.h>

//******************************************************************************
// Data types

typedef char s8;
typedef unsigned char u8;
typedef int s16;
typedef unsigned int u16;
typedef long s32;
typedef unsigned long u32;
typedef long long s64;
typedef unsigned long long u64;

// *****************************************************************************
// CPU clock (Hz)

#define F_CPU                             8000000UL

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

