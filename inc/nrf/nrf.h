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
#define NRF_CMD_ACTIVATE      0x50
#define NRF_CMD_R_RX_PL_WID   0x60
#define NRF_CMD_W_ACK_PAYLOAD 0xA8
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
#define NRF_REG_TX_ADDR       0x10
#define NRF_REG_RX_PW_P0      0x11
#define NRF_REG_RX_PW_P1      0x12
#define NRF_REG_RX_PW_P2      0x13
#define NRF_REG_RX_PW_P3      0x14
#define NRF_REG_RX_PW_P4      0x15
#define NRF_REG_RX_PW_P5      0x16
#define NRF_REG_RX_PW( n )    ( ( n ) + NRF_REG_RX_PW_P0 )
#define NRF_REG_FIFO_STATUS   0x17
#define NRF_REG_DYNPD         0x1C
#define NRF_REG_FEATURE       0x1D

// CONFIG reg constants
#define NRF_CONFIG_CRC_1BYTE  0
#define NRF_CONFIG_CRC_2BYTES 1
#define NRF_CONFIG_MODE_PTX   0
#define NRF_CONFIG_MODE_PRX   1

// STAT reg constants
#define NRF_STAT_RX_P_NONE    6
#define NRF_STAT_RX_P_EMPTY   7
#define NRF_INT_MAX_RT_MASK   0x10
#define NRF_INT_TX_DS_MASK    0x20
#define NRF_INT_RX_DR_MASK    0x40

// SETUP_AW reg constants
#define NRF_SETUP_AW_SIZE_3   1
#define NRF_SETUP_AW_SIZE_4   2
#define NRF_SETUP_AW_SIZE_5   3

// RF_SETUP reg constants
#define NRF_RF_SETUP_DR_1MBPS 0
#define NRF_RF_SETUP_DR_2MBPS 1
#define NRF_RF_SETUP_PWR_0    3
#define NRF_RF_SETUP_PWR_M6   2
#define NRF_RF_SETUP_PWR_M12  1
#define NRF_RF_SETUP_PWR_M18  0
#define NRF_RF_SETUP_LNA_ON   1
#define NRF_RF_SETUP_LNA_OFF  0

// Other constants
#define NRF_PAYLOAD_SIZE      32
#define NRF_MODE_RX           1
#define NRF_MODE_TX           0
// Server pipe 0 address (LSB to MSB) -> only for server 
#define NRF_CFG_SRV_PIPE0_ADDR  { 0xA1, 0x32, 0x58, 0xF3, 0xC9 }
// Server pipe 1 address (LSB to MSB) -> only for server
#define NRF_CFG_SRV_PIPE1_ADDR  { 0xD4, 0x77, 0xC5, 0xEA, 0xB0 }

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
void nrf_activate();
int nrf_get_payload_size();
void nrf_write_ack_payload( const u8 *dataptr, u16 len );

// Higher level nRF commands
void nrf_set_setup_retr( unsigned delay, unsigned count );
void nrf_set_rf_setup( int data_rate, int pwr, int lna );
void nrf_set_rx_addr( int pipe, const u8* paddr );
void nrf_get_rx_addr( int pipe, u8 *addrbuf );
void nrf_set_tx_addr( const u8 *paddr );
void nrf_set_payload_size( int pipe, u8 size );
void nrf_set_mode( int mode );
void nrf_clear_interrupt( u8 mask );
int nrf_has_data();
unsigned nrf_send_packet( const u8 *addr, const u8 *pdata, unsigned len );
unsigned nrf_get_packet( u8 *pdata, unsigned maxlen, int *pipeno );

// Other public functions
void nrf_init();
void nrf_irq_handler();

#endif

