// nRF configuration

#ifndef __NRF_CONF_H__
#define __NRF_CONF_H__

#include "nrf.h"

// RF channel
#define NRF_CFG_RF_CHANNEL    2
// TX power (0 is the highest, see NRF_RF_SETUP_PWR_xxx in nrf.h for more values)
#define NRF_CFG_TX_POWER      NRF_RF_SETUP_PWR_0
// Data rate (1Mbps or 2Mbps)
#define NRF_CFG_DATA_RATE     NRF_RF_SETUP_DR_1MBPS
// Debugging on
//#define NRF_DEBUG

#endif // #ifndef __NRF_CONF_H__

