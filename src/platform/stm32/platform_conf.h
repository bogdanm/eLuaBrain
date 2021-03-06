// eLua platform configuration

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#include "auxmods.h"
#include "type.h"
#include "stacks.h"
#include "stm32f10x.h"
#include "elua_int.h"
#include "sermux.h"
#include "buf.h"

// *****************************************************************************
// Define here what components you want for this platform

//#define BUILD_XMODEM
#define BUILD_SHELL
#define BUILD_ROMFS
#define BUILD_MMCFS
#define BUILD_TERM_VRAM
//#define BUILD_TERM
#define BUILD_UIP
#define BUILD_DHCPC
#define BUILD_DNS
#define BUILD_CON_GENERIC
#define BUILD_ADC
//#define BUILD_RPC
#define BUILD_RFS
//#define BUILD_CON_TCP
#define BUILD_VRAM
#define BUILD_LINENOISE
#define BUILD_C_INT_HANDLERS
#define BUILD_LUA_INT_HANDLERS
#define BUILD_PS2
#define BUILD_ENC28J60
#define BUILD_NRF
#define BUILD_HELP

#define MMCFS_SDIO_STM32
#define RFS_TRANSPORT_UDP

// *****************************************************************************
// UART/Timer IDs configuration data (used in main.c)

//#define CON_UART_ID           0
//#define CON_UART_SPEED        115200
//#define CON_TIMER_ID          0
#define TERM_LINES            30
#define TERM_COLS             80

// *****************************************************************************
// Auxiliary libraries that will be compiled for this platform

#define PS_LIB_TABLE_NAME     "stm32"

#ifdef BUILD_ADC
#define ADCLINE _ROM( AUXLIB_ADC, luaopen_adc, adc_map )
#else
#define ADCLINE
#endif

#if defined( ELUA_BOOT_RPC ) && !defined( BUILD_RPC )
#define BUILD_RPC
#endif

#if defined( BUILD_RPC ) 
#define RPCLINE _ROM( AUXLIB_RPC, luaopen_rpc, rpc_map )
#else
#define RPCLINE
#endif

#ifdef PS_LIB_TABLE_NAME
#define PLATLINE _ROM( PS_LIB_TABLE_NAME, luaopen_platform, platform_map )
#else
#define PLATLINE
#endif

#define LUA_PLATFORM_LIBS_ROM\
  _ROM( AUXLIB_PIO, luaopen_pio, pio_map )\
  _ROM( AUXLIB_SPI, luaopen_spi, spi_map )\
  _ROM( AUXLIB_PD, luaopen_pd, pd_map )\
  _ROM( AUXLIB_UART, luaopen_uart, uart_map )\
  _ROM( AUXLIB_TERM, luaopen_term, term_map )\
  _ROM( AUXLIB_PACK, luaopen_pack, pack_map )\
  _ROM( AUXLIB_BIT, luaopen_bit, bit_map )\
  _ROM( AUXLIB_CPU, luaopen_cpu, cpu_map )\
  _ROM( AUXLIB_TMR, luaopen_tmr, tmr_map )\
  _ROM( AUXLIB_I2C, luaopen_i2c, i2c_map )\
  ADCLINE\
  _ROM( AUXLIB_CAN, luaopen_can, can_map )\
  _ROM( AUXLIB_PWM, luaopen_pwm, pwm_map )\
  RPCLINE\
  _ROM( AUXLIB_ELUA, luaopen_elua, elua_map )\
  _ROM( LUA_MATHLIBNAME, luaopen_math, math_map )\
  _ROM( AUXLIB_NET, luaopen_net, net_map)\
  _ROM( AUXLIB_NRF, luaopen_nrf, nrf_map)\
  PLATLINE

// *****************************************************************************
// Static TCP/IP configuration

// Static TCP/IP configuration
#define ELUA_CONF_IPADDR0     192
#define ELUA_CONF_IPADDR1     168
#define ELUA_CONF_IPADDR2     1
#define ELUA_CONF_IPADDR3     200

#define ELUA_CONF_NETMASK0    255
#define ELUA_CONF_NETMASK1    255
#define ELUA_CONF_NETMASK2    255
#define ELUA_CONF_NETMASK3    0

#define ELUA_CONF_DEFGW0      192
#define ELUA_CONF_DEFGW1      168
#define ELUA_CONF_DEFGW2      1
#define ELUA_CONF_DEFGW3      1

#define ELUA_CONF_DNS0        192
#define ELUA_CONF_DNS1        168
#define ELUA_CONF_DNS2        1
#define ELUA_CONF_DNS3        1

// *****************************************************************************
// Configuration data

#define EGC_INITIAL_MODE      1

// Virtual timers (0 if not used)
#define VTMR_NUM_TIMERS       4
#define VTMR_FREQ_HZ          10

// Number of resources (0 if not available/not implemented)
#define NUM_PIO               7
#define NUM_SPI               3
#define NUM_UART              4
#define NUM_TIMER             5
#define NUM_PWM               4
#define NUM_ADC               16
#define NUM_CAN               1
#define NUM_I2C               2

// Enable RX buffering on UART
#define BUF_ENABLE_UART
//#define CON_BUF_SIZE          BUF_SIZE_128

// ADC Configuration Params
#define ADC_BIT_RESOLUTION    12
#define BUF_ENABLE_ADC
#define ADC_BUF_SIZE          BUF_SIZE_2

// These should be adjusted to support multiple ADC devices
#define ADC_TIMER_FIRST_ID    0
#define ADC_NUM_TIMERS        4

// RPC boot options
#define RPC_UART_ID           CON_UART_ID
#define RPC_TIMER_ID          CON_TIMER_ID
#define RPC_UART_SPEED        CON_UART_SPEED

// MMCFS Support (FatFs on SD/MMC)
// For STM32F103RET6 - PA5 = CLK, PA6 = MISO, PA7 = MOSI, PA8 = CS
#define MMCFS_TICK_HZ                10
#define MMCFS_TICK_MS                ( 1000 / MMCFS_TICK_HZ )
#define MMCFS_CS_PORT                0
#define MMCFS_CS_PIN                 8
#define MMCFS_SPI_NUM                0
#define MMCFS_CARD_PIN               11
#define MMCFS_CARD_PORT              5

// PS/2 configuration
#define PS2_DATA_PORT         6
#define PS2_DATA_PIN          13
#define PS2_CLOCK_PORT        6
#define PS2_CLOCK_PIN         15
#define PS2_TIMER_ID          0
#define PS2_RESET_PORT        6
#define PS2_RESET_PIN         14

// ENC28J60 configuration
#define ENC28J60_SPI_ID       0
#define ENC28J60_CS_PORT      3  
#define ENC28J60_CS_PIN       3
#define ENC28J60_MAC_ADDRESS  { 0x00, 0x30, 0x84, 0x25, 0xE6, 0x1D }
#define ENC28J60_SPI_CLOCK    8000000
#define ENC28J60_RESET_PORT   0
#define ENC28J60_RESET_PIN    13
#define ENC28J60_INT_PORT     6
#define ENC28J60_INT_PIN      7

// nRF24L01 configuration
#define NRF24L01_CE_PORT      6
#define NRF24L01_CE_PIN       11 
#define NRF24L01_CSN_PORT     6
#define NRF24L01_CSN_PIN      12
#define NRF24L01_IRQ_PORT     6
#define NRF24L01_IRQ_PIN      8
#define NRF24L01_UART_ID      1
#define NRF24L01_BUF_SIZE     BUF_SIZE_1024
#define NRF24L01_TMR_ID       0 // [TODO] change this?
#define NRF24L01_TIMEOUT      100000

// Sound resources
#define SND_PWM_ID            1          
#define SND_TIMER_ID          ( VTMR_FIRST_ID + 3 )

// CPU frequency (needed by the CPU module, 0 if not used)
u32 platform_s_cpu_get_frequency();
#define CPU_FREQUENCY         platform_s_cpu_get_frequency()

// PIO prefix ('0' for P0, P1, ... or 'A' for PA, PB, ...)
#define PIO_PREFIX            'A'
// Pins per port configuration:
// #define PIO_PINS_PER_PORT (n) if each port has the same number of pins, or
// #define PIO_PIN_ARRAY { n1, n2, ... } to define pins per port in an array
// Use #define PIO_PINS_PER_PORT 0 if this isn't needed
#define PIO_PINS_PER_PORT     16

// Remote file system data
#define RFS_BUFFER_SIZE       BUF_SIZE_512
//#define RFS_UART_ID           0
#define RFS_TIMER_ID          2
#define RFS_TIMEOUT           400000
//#define RFS_UART_SPEED        115200

// Linenoise buffer sizes
#define LINENOISE_HISTORY_SIZE_LUA    50
#define LINENOISE_HISTORY_SIZE_SHELL  10

// Allocator data: define your free memory zones here in two arrays
// (start address and end address)
#define SRAM_SIZE             ( 64 * 1024 )
#define EXTSRAM_START         0x68000000
#define EXTSRAM_SIZE          ( 512 * 2 * 1024 )
#define MEM_START_ADDRESS     { ( void* )end, ( void* )EXTSRAM_START }
#define MEM_END_ADDRESS       { ( void* )( SRAM_BASE + SRAM_SIZE - STACK_SIZE_TOTAL - 1 ), ( void* )( EXTSRAM_START + EXTSRAM_SIZE - 1 ) }
//#define MEM_START_ADDRESS     { ( void* )end }
//#define MEM_END_ADDRESS       { ( void* )( SRAM_BASE + SRAM_SIZE - STACK_SIZE_TOTAL - 1 ) }

// Interrupt queue size
#define PLATFORM_INT_QUEUE_LOG_SIZE 5

// Interrupt list
#define INT_GPIO_POSEDGE      ELUA_INT_FIRST_ID
#define INT_GPIO_NEGEDGE      ( ELUA_INT_FIRST_ID + 1 )
#define INT_TMR_MATCH         ( ELUA_INT_FIRST_ID + 2 )
#define INT_UART_RX           ( ELUA_INT_FIRST_ID + 3 )
#define INT_ELUA_LAST         INT_UART_RX

#define PLATFORM_CPU_CONSTANTS\
  _C( INT_GPIO_POSEDGE ),     \
  _C( INT_GPIO_NEGEDGE ),     \
  _C( INT_TMR_MATCH ),        \
  _C( INT_UART_RX )

#endif // #ifndef __PLATFORM_CONF_H__

