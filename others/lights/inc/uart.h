// Asynchronous USART driver

#ifndef __UART_H__
#define __UART_H__

#include "machine.h"

void uart_init();
void uart_putchar( u8 c );
s16 uart_nb_getchar();
u8 uart_getchar();
u8 uart_getchar_to( u8 *, u16 ); 
void uart_empty_rx_buffer();
void uart_empty_tx_buffer();
u8 uart_bytes_received();

#endif
