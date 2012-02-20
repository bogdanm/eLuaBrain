// SIRCS remote protocol data

#ifndef __SIRCS_H__
#define __SIRCS_H__

#include "type.h"
#include "platform_conf.h"
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "common.h"

// Pin definitions (PB9, TIM4.CH4)
#define DECODER_TIM             TIM4
#define DECODER_TIM_IRQn        TIM4_IRQn
#define DECODER_IRQHANDLER_NAME TIM4_IRQHandler
#define DECODER_TIM_IDX         3                 

#define DECODER_TIM_CH          4
#define DECODER_TIM_CH_DEF      TIM_Channel_4
#define DECODER_TIM_CH_INT_DEF  TIM_IT_CC4
#define DECODER_TIM_CH_CCR      CCR4

#define DECODER_PIN             9
#define DECODER_BASE_PORT       GPIOB_BASE

#define DECODER_INTERKEY_TIMERID    ( VTMR_FIRST_ID )
#define DECODER_BASE_CLOCK      100000

#define DECODER_TIM_CH_CCER_SHIFT   ( 1 + ( DECODER_TIM_CH - 1 ) * 4 )

// Bit banding data
#define BB_PERIPH_ALIAS_BASE    0x42000000
#define BB_PERIPH_BASE          0x40000000
#define GPIOPIN_IDR_ADDR        ( DECODER_BASE_PORT + 0x08 )
#define BB_DECODER_PIN_ADDR     ( BB_PERIPH_ALIAS_BASE + ( GPIOPIN_IDR_ADDR - BB_PERIPH_BASE ) * 32 + DECODER_PIN * 4 )
#define BB_DECODER              *( volatile u32* )BB_DECODER_PIN_ADDR

// Time limits
#define DECODER_START_BIT_TIME  240
#define DECODER_SPACE_TIME      60
#define DECODER_ONE_TIME        120
#define DECODER_ZERO_TIME       60
#define DECODER_BIT_ERR         10

// Key decoding
#define DECODER_DEVID_MASK      0x0F80
#define DECODER_DEVID_SHIFT     7
#define DECODER_KEYCODE_MASK    0x007F
#define DECODER_KEYCODE_SHIFT   0      

// Decoder queue
#define DECODER_QUEUE_SIZE      32  // must be a power of 2

// Minimum delay between 2 consecutive keys in virtual clock units 
#define DECODER_INTERKEY_MINDELAY   ( 200 / ( 1000 / VTMR_FREQ_HZ ) )

// How many retries of the designated "reset" key are needed to reset the board
#define SIRCS_RESET_COUNT       2

// Macro for declaring keys/values
#define SIRCS_DATA( key, value )  key = value

// Initialize the SIRCS decoder
void sircs_init();

#endif
