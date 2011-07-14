// nRF configuration

#ifndef __NRF_CONF_H__
#define __NRF_CONF_H__

#include "nrf.h"

// Profile (server or client)
#define NRF_CFG_PROFILE_SERVER
// RF channel
#define NRF_CFG_RF_CHANNEL    2
// TX power (0 is the highest, see NRF_RF_SETUP_PWR_xxx in nrf.h for more values)
#define NRF_CFG_TX_POWER      NRF_RF_SETUP_PWR_0
// Data rate (1Mbps or 2Mbps)
#define NRF_CFG_DATA_RATE     NRF_RF_SETUP_DR_1MBPS
// Server pipe 0 address (LSB to MSB) -> only for server 
#define NRF_CFG_SRV_PIPE0_ADDR  { 0xA1, 0x32, 0x58, 0xF3, 0xC9 }
// Server pipe 1 address (LSB to MSB) -> only for server
#define NRF_CFG_SRV_PIPE1_ADDR  { 0xA2, 0x77, 0xC5, 0xEA, 0xB0 }

#endif // #ifndef __NRF_CONF_H__

