// SIRCS decoder

#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "common.h"
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "modcommon.h"
#include "lrotable.h"
#include "sircs.h"
#include "sony_rm_dm9.h"
#include "stm32f10x_wwdg.h"
#include "stm32f10x_rcc.h"

enum
{
  SONY_RM_DM9_KEYS
};

// *****************************************************************************
// Decoder variables and macros

enum
{
  DECODER_STATE_WAIT_START,
  DECODER_STATE_IN_START_1,
  DECODER_STATE_IN_START_0,
  DECODER_STATE_DECODING,
  DECODER_STATE_SPACE
};

static int decoder_state;
static int decoder_nbits;   
static int decoder_code;
static timer_data_type decoder_add_time;
static timer_data_type decoder_crt_time;
static int decoder_reset_count;

// Decoder queue
static u16 decoder_queue[ DECODER_QUEUE_SIZE ];
static volatile u16 decoder_r_idx, decoder_w_idx; 

// Reset line data
#define PLATFORM_RESET_PORT             2
#define PLATFORM_RESET_PIN              12

// *****************************************************************************
// Decoder queue helpers

static u16 sircs_queue_get_size()
{
  return ( decoder_w_idx + DECODER_QUEUE_SIZE - decoder_r_idx ) & ( DECODER_QUEUE_SIZE - 1 );
}

static int sircs_queue_has_data()
{
  return decoder_r_idx != decoder_w_idx;
}

static int sircs_queue_get_data()
{
  int data = -1;
  
  if( decoder_r_idx != decoder_w_idx )
  {
    data = decoder_queue[ decoder_r_idx ];
    decoder_r_idx = ( decoder_r_idx + 1 ) & ( DECODER_QUEUE_SIZE - 1 );
  }
  return data;
   
}

static void sircs_queue_add_data( u16 data )
{
  if( sircs_queue_get_size() < DECODER_QUEUE_SIZE )
  {
    decoder_queue[ decoder_w_idx ] = decoder_code;
    decoder_w_idx = ( decoder_w_idx + 1 ) & ( DECODER_QUEUE_SIZE - 1 );  
  }
}

// *****************************************************************************
// Utility functions
static void system_reset()
{
  volatile unsigned i;
  
  platform_pio_op( PLATFORM_RESET_PORT, 1 << PLATFORM_RESET_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( PLATFORM_RESET_PORT, 1 << PLATFORM_RESET_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );     
  for( i = 0; i < 1000000; i ++ );  
  platform_pio_op( PLATFORM_RESET_PORT, 1 << PLATFORM_RESET_PIN, PLATFORM_IO_PIN_DIR_INPUT );
  while( 1 ); // the W7100 will reset the STM32 at this point  
}

// *****************************************************************************
// Decoder implementation

void sircs_init()
{
  TIM_ICInitTypeDef  TIM_ICInitStructure;  
  NVIC_InitTypeDef nvic_init_structure;
  
  nvic_init_structure.NVIC_IRQChannel = DECODER_TIM_IRQn;
  nvic_init_structure.NVIC_IRQChannelSubPriority = 0;
  nvic_init_structure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &nvic_init_structure );  
    
  RCC_APB1PeriphClockCmd( RCC_APB1Periph_WWDG, ENABLE );
  
  platform_timer_op( DECODER_TIM_IDX, PLATFORM_TIMER_OP_SET_CLOCK, DECODER_BASE_CLOCK );
  TIM_ICInitStructure.TIM_Channel = DECODER_TIM_CH_DEF;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit( DECODER_TIM, &TIM_ICInitStructure );
    
  DECODER_TIM->SR = 0;
  TIM_ITConfig( DECODER_TIM, DECODER_TIM_CH_INT_DEF, ENABLE );     
}

void DECODER_IRQHANDLER_NAME()
{
  u32 cnt = DECODER_TIM->DECODER_TIM_CH_CCR;
  
  DECODER_TIM->CNT = 0; 
  switch( decoder_state )
  {
    case DECODER_STATE_WAIT_START:
      // 1 to 0 transition, before startbit
      if( BB_DECODER == 0 )
        decoder_state = DECODER_STATE_IN_START_1;
      else
        goto set_start_state;
      break;
      
    case DECODER_STATE_IN_START_1:
      // 0 to 1 transition, was startbit
      if( BB_DECODER == 1 && cnt >= DECODER_START_BIT_TIME - DECODER_BIT_ERR && cnt <= DECODER_START_BIT_TIME + DECODER_BIT_ERR )
        decoder_state = DECODER_STATE_IN_START_0;
      else
        goto set_start_state;
      break;
      
    case DECODER_STATE_IN_START_0:
      // 1 to 0 transition, was space of start bit
      if( BB_DECODER == 0 && cnt >= DECODER_SPACE_TIME - DECODER_BIT_ERR && cnt <= DECODER_SPACE_TIME + DECODER_BIT_ERR )
        decoder_state = DECODER_STATE_DECODING;
      else
        goto set_start_state;
      break;
      
    case DECODER_STATE_DECODING:
      // 0 to 1 trasition, was data bit
      if( BB_DECODER == 1 )
      {
        if( cnt >= DECODER_ONE_TIME - DECODER_BIT_ERR && cnt <= DECODER_ONE_TIME + DECODER_BIT_ERR )
          decoder_code |= 1 << 11;
        else if( cnt < DECODER_ZERO_TIME - DECODER_BIT_ERR || cnt > DECODER_ZERO_TIME + DECODER_BIT_ERR )
          goto set_start_state;          
        decoder_nbits ++;
        decoder_state = DECODER_STATE_SPACE;
      }
      else
        goto set_start_state;      
      break;
      
    case DECODER_STATE_SPACE:
      // 1 to 0 transition, was space
      if( decoder_nbits == 12 )
      {
        decoder_crt_time = platform_timer_op( DECODER_INTERKEY_TIMERID, PLATFORM_TIMER_OP_READ, 0 );
        if( decoder_crt_time - decoder_add_time >= DECODER_INTERKEY_MINDELAY )
        {
          sircs_queue_add_data( decoder_code );
          if( ( ( decoder_code & DECODER_KEYCODE_MASK ) >> DECODER_KEYCODE_SHIFT ) == SIRCS_RESET_KEY )
          {
            decoder_reset_count ++;
            if( decoder_reset_count == SIRCS_RESET_COUNT )
              system_reset();
          }
          else
            decoder_reset_count = 0;
          decoder_add_time = decoder_crt_time;
        }        
        goto set_start_state; 
      }    
      else
        decoder_code >>= 1;
      // 1 to 0 transition, was space
      if( BB_DECODER == 0 && cnt >= DECODER_SPACE_TIME - DECODER_BIT_ERR && cnt <= DECODER_SPACE_TIME + DECODER_BIT_ERR )
        decoder_state = DECODER_STATE_DECODING;
      else
        goto set_start_state;      
      break;
  }
  DECODER_TIM->CCER ^= ( 1 << DECODER_TIM_CH_CCER_SHIFT );  
  return;

set_start_state:
  decoder_state = DECODER_STATE_WAIT_START;
  decoder_code = 0;
  decoder_nbits = 0;    
  DECODER_TIM->CCER |= ( 1 << DECODER_TIM_CH_CCER_SHIFT );
}

// *****************************************************************************
// sircs lua module implementation

// Lua: haskey = sircs.haskey()
static int sircs_haskey( lua_State *L )
{
  lua_pushboolean( L, sircs_queue_has_data() );
  return 1;
} 

// Lua: size = sircs.getsize()
static int sircs_getsize( lua_State *L )
{
  lua_pushinteger( L, sircs_queue_get_size() );
  return 1;
}

// Lua: sircs.empty()
static int sircs_empty( lua_State *L )
{
  while( sircs_queue_has_data() )
    sircs_queue_get_data();
  return 0;
}

// Lua: devid, keycode = sircs.getkey( blocking )
static int sircs_getkey( lua_State *L )
{
  int data;
  unsigned devid, keycode;
  int blocking = 1;
  
  if( lua_isboolean( L, 1 ) )
    blocking = lua_toboolean( L, 1 );
  else
    return luaL_error( L, "expected a bool argument" );
  if( blocking )
    while( sircs_queue_get_size() == 0 );
  data = sircs_queue_get_data();
  if( data == -1 )
  {
    lua_pushnil( L );
    return 1;
  }
  else
  {
    devid = ( data & DECODER_DEVID_MASK ) >> DECODER_DEVID_SHIFT;
    keycode = ( data & DECODER_KEYCODE_MASK ) >> DECODER_KEYCODE_SHIFT;
    lua_pushinteger( L, devid );
    lua_pushinteger( L, keycode );
    return 2;  
  }
}


#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE sircs_map[] =
{
  { LSTRKEY( "haskey" ), LFUNCVAL( sircs_haskey ) },
  { LSTRKEY( "getsize" ), LFUNCVAL( sircs_getsize ) },
  { LSTRKEY( "empty" ), LFUNCVAL( sircs_empty ) },
  { LSTRKEY( "getkey" ), LFUNCVAL( sircs_getkey ) },
#if LUA_OPTIMIZE_MEMORY > 0
#undef SIRCS_DATA
#define SIRCS_DATA( key, code ) { LSTRKEY( #key ), LNUMVAL( code ) }
  SONY_RM_DM9_KEYS,      
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_sircs( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
#error "sircs not implemented properly in LUA_OPTIMIZE_MEMORY = 0"
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}

