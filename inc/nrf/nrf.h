// nRF functions

#ifndef __NRF_H__
#define __NRF_H__

#include "type.h"

// Commands
#define NRF_CMD_R_REGISTER    0x00
#define NRF_CMD_W_REGISTER    0x20
#define NRF_CMD_R_RX_PAYLOAD  0x61
#define NRF_CMD_W_TX_PAYLOAD  0xA0
#define NRF_CMD_FLUSH_TX      0xE1
#define NRF_CMD_FLUSH_RX      0xE2
#define NRF_CMD_REUSE_TX_PL   0xE3
#define NRF_CMD_NOP           0xFF

// Register addresses
#define NRF_REG_CONFIG        0x00
#define NRF_REG_EN_AA         0x01
#define NRF_REG_EN_RXADDR     0x02
#define NRF_REG_SETUP_AW      0x03
#define NRF_REG_SETUP_RETR    0x04
#define NRF_REG_RF_CH         0x05
#define NRF_REG_RF_SETUP      0x06
#define NRF_REG_STATUS        0x07
#define NRF_REG_OBSERVE_TX    0x08
#define NRF_REG_CD            0x09
#define NRF_REG_RX_ADDR_P0    0x0A
#define NRF_REG_RX_ADDR_P1    0x0B
#define NRF_REG_RX_ADDR_P2    0x0C
#define NRF_REG_RX_ADDR_P3    0x0D
#define NRF_REG_RX_ADDR_P4    0x0E
#define NRF_REG_RX_ADDR_P5    0x0F
#define NRF_REG_RX_ADDR( n )  ( ( n ) + NRF_REG_RX_ADDR_P0 )
#define NRF_REG_RX_ADDR       0x10
#define NRF_REG_RX_PW_P0      0x11
#define NRF_REG_RX_PW_P1      0x12
#define NRF_REG_RX_PW_P2      0x13
#define NRF_REG_RX_PW_P3      0x14
#define NRF_REG_RX_PW_P4      0x15
#define NRF_REG_RX_PW_P5      0x16
#define NRF_REG_RW_PW( n )    ( ( n ) + NRF_REG_RX_PW_P0 )
#define NRF_REG_FIFO_STATUS   0x17

// Status register (mask, shift) pairs
#define NRF_STAT_TX_FULL_M    1
#define NRF_STAT_TX_FULL_S    0
#define NRF_STAT_RX_P_NO_M    7
#define NRF_STAT_RX_P_NO_S    1
#

// Configuration data
#define NRF_CFG_PAYLOAD_SIZE  32
#define NRF_CFG_ADDR_SIZE     5

// Basic nRF commands
void nrf_read_register( u8 address, u8 *dataptr, u16 len );
u8 nrf_read_register_byte( u8 address );
void nrf_write_register( u8 address, const u8 *dataptr, u16 len );
void nrf_write_register_byte( u8 address, u8 data );
void nrf_get_rx_payload( u8 *dataptr, u16 len );
void nrf_write_tx_payload( const u8 *dataptr, u16 len );
void nrf_flush_rx();
void nrf_flush_tx();
u8 nrf_get_status();

// Higher level nRF commands
void nrf_set_rx_addr( u8 pipe, const u8* paddr );
void nrf_set_tx_addr( const u8 *paddr );
void nrf_set_payload_size( u8 pipe, u8 size );
void nrf_set_interrupt( 

// Other public functions
void nrf_init();

#endif

