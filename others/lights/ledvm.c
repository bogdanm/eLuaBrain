// LED virtual machine 

#include "ledvm.h"
#include "type.h"
#include <string.h>
#include <stdio.h>

// ****************************************************************************
// Local variables and definitionsA

#define VM_INC_PC             vm_d.pc ++
#define CH_RED                0
#define CH_GREEN              1
#define CH_BLUE               2

// Internal data of the virtual machine
typedef struct {
  u16 pc;                     // program counter
  s8 v[ 3 ];                  // current reg, green, blue values
  s8 a[ 3 ];                  // linear "a" coeficients
  s8 b[ 3 ];                  // linear "b" coeficients
  u8 m[ 3 ];                  // color channel masks
  s8 min[ 3 ];                // minimum values
  s8 max[ 3 ];                // maximum values
  u8 on[ 3 ];                 // channel on/off flags
  u16 cnt_crt[ LEDVM_REP_COUNTERS ]; // current counter values
  u16 cnt_saddr[ LEDVM_REP_COUNTERS ] ; // counter start addresses
  u8 cnt_active;              // active counter bitmap
  u8 cnt_lim;                 // 'limit' mode bitmap for counters
  u32 rgb_delay;              // default delay for RGB set ops
  u16 cstack[ LEDVM_MAX_CALLS ];  // call stack
  u8 numcalls;                // call stack depth
} LEDVM_DATA;

typedef int ( *p_handler )( const u8* );
static LEDVM_DATA vm_d;

// ****************************************************************************
// Local functions and helpers

static s8 vmh_clip_rgb( int ch, int v )
{
  if( v < vm_d.min[ ch ] )
    v = vm_d.min[ ch ];
  if( v > vm_d.max[ ch ] )
    v = vm_d.max[ ch ];
  return ( s8 )v;
}

static void vmh_set_rgbdata( int r, int g, int b, int dodelay )
{
  vm_d.v[ CH_RED ] = vmh_clip_rgb( CH_RED, vm_d.on[ CH_RED ] ? r : 0 );
  vm_d.v[ CH_GREEN ] = vmh_clip_rgb( CH_GREEN, vm_d.on[ CH_GREEN ] ? g : 0 );
  vm_d.v[ CH_BLUE ] = vmh_clip_rgb( CH_BLUE, vm_d.on[ CH_BLUE ] ? b : 0 );
  ledvm_ll_setrgb( vm_d.v[ CH_RED ], vm_d.v[ CH_GREEN ], vm_d.v[ CH_BLUE ] );
  if( dodelay )
    ledvm_ll_delayms( vm_d.rgb_delay );
}

static void vmh_set_crtrgb( int dodelay )
{
  vmh_set_rgbdata( vm_d.v[ CH_RED ], vm_d.v[ CH_GREEN ], vm_d.v[ CH_BLUE ], dodelay );
}

static u32 vmh_data_to_u32( const u8 *pdata )
{
  return ( ( u32 )pdata[ 1 ] << 16 ) | ( ( u32 )pdata[ 2 ] << 8 ) | pdata[ 3 ];
}

static int vmh_check_empty_inst( const u8 *pdata )
{
  return vmh_data_to_u32( pdata ) == LEDVM_EMPTY_MARK;
}

static s8 vmh_getv( u8 ch, int v, int specvalues )
{
  u8 mask = vm_d.m[ ch ];
  s8 crt = vm_d.v[ ch ];

  if( mask == 0 )
    return crt;
  if( specvalues && v == LEDVM_NUM_RAND )
    return ledvm_ll_rand() % 33;
  else if( specvalues && v == LEDVM_NUM_SAME )
    return crt;
  else
    return v;
}

static void vmh_check_mask( u8 data )
{
  if( data & LEDVM_MASKNEW_BIT )
  {
    vm_d.m[ CH_RED ] = ( data & LEDVM_MASKR_BIT ) != 0;
    vm_d.m[ CH_GREEN ] = ( data & LEDVM_MASKG_BIT ) != 0;
    vm_d.m[ CH_BLUE ] = ( data & LEDVM_MASKB_BIT ) != 0;
  }
}

// ----------------------------------------------------------------------------
// Instruction handlers

static int vm_h_nop( const u8 *pdata )
{
  if( !vmh_check_empty_inst( pdata ) )
    return LEDVM_ERR_INVALID_INSTRUCTION;
  vmh_check_mask( pdata[ 0 ] );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_setrgb( const u8 *pdata ) 
{
  vmh_check_mask( pdata[ 0 ] );
  u8 r = vmh_getv( CH_RED, pdata[ 1 ], 1 );
  u8 g = vmh_getv( CH_GREEN, pdata[ 2 ], 1 );
  u8 b = vmh_getv( CH_BLUE, pdata[ 3 ], 1 );

  vmh_set_rgbdata( r, g, b, 1 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_delay( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  ledvm_ll_delayms( vmh_data_to_u32( pdata ) );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_seta( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  memcpy( vm_d.a, pdata + 1, 3 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_setb( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  memcpy( vm_d.b, pdata + 1, 3 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_repeat( const u8 *pdata )
{
  u8 cntid = pdata[ 0 ] & 0x0F;
  u16 cnt, addr;

  if( vm_d.cnt_active & ( 1 << cntid ) ) // counter already active
  {
    cnt = 0; // 'disable counter' flag
    if( vm_d.cnt_lim & ( 1 << cntid ) ) // limit mode?
    {
      for( addr = 0; addr < 3; addr ++ ) 
        if( vm_d.m[ addr ] && ( vm_d.v[ addr ] <= vm_d.min[ addr ] || vm_d.v[ addr ] >= vm_d.max[ addr ] ) )
        {
          cnt = 1;
          break;
        }
    }
    else // regular mode (no limiting)
    { 
      if( -- vm_d.cnt_crt[ cntid ] == 0 ) // more iterations to go?
        cnt = 1;
    }
    if( cnt ) // disable counter?
    {
      vm_d.cnt_active &= ( u8 )~( 1 << cntid );
      vm_d.cnt_lim &= ( u8 )~( 1 << cntid );
      VM_INC_PC;
    }
    else
      vm_d.pc = vm_d.cnt_saddr[ cntid ];
  }
  else // counter not active
  {
    cnt = ( ( u16 )pdata[ 1 ] << 4 ) | ( pdata[ 2 ] >> 4 );
    addr = ( ( ( u16 )pdata[ 2 ] & 0x0F ) << 8 ) | pdata[ 3 ];
    vm_d.cnt_crt[ cntid ] = cnt;
    vm_d.pc = vm_d.cnt_saddr[ cntid ] = addr;
    vm_d.cnt_active |= 1 << cntid;
    if( cnt == LEDVM_CNT_LIM )
      vm_d.cnt_lim |= 1 << cntid;
  }
  return LEDVM_ERR_OK;
}

static int vm_h_rgbdelay( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  vm_d.rgb_delay = vmh_data_to_u32( pdata );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_nextrgb( const u8 *pdata )
{
  int newv[ 3 ], i;

  if( !vmh_check_empty_inst( pdata ) )
    return LEDVM_ERR_INVALID_INSTRUCTION;
  vmh_check_mask( pdata[ 0 ] );
  for( i = 0; i < 3; i ++ )
    newv[ i ] = ( int )vm_d.a[ i ] * vm_d.v[ i ] + vm_d.b[ i ];
  vmh_set_rgbdata( vmh_getv( CH_RED, newv[ CH_RED ], 0 ),
                   vmh_getv( CH_GREEN, newv[ CH_GREEN ], 0 ),
                   vmh_getv( CH_BLUE, newv[ CH_BLUE ], 0 ),
                   1 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_call( const u8 *pdata )
{
  if( vm_d.numcalls == LEDVM_MAX_CALLS )
    return LEDVM_ERR_TOO_MANY_CALLS;
  vmh_check_mask( pdata[ 0 ] );
  vm_d.cstack[ vm_d.numcalls ++ ] = vm_d.pc + 1;
  vm_d.pc = vmh_data_to_u32( pdata );
  return LEDVM_ERR_OK;
}

static int vm_h_ret( const u8 *pdata )
{
  if( !vmh_check_empty_inst( pdata ) )
    return LEDVM_ERR_INVALID_INSTRUCTION;
  if( vm_d.numcalls == 0 )
    return LEDVM_ERR_INVALID_RET;
  vmh_check_mask( pdata[ 0 ] );
  vm_d.pc = vm_d.cstack[ -- vm_d.numcalls ];
  return LEDVM_ERR_OK;
}

static int vm_h_addrgb( const u8 *pdata )
{
  int newv[ 3 ], i;

  vmh_check_mask( pdata[ 0 ] );
  for( i = 0; i < 3; i ++ )
    newv[ i ] = ( int )vm_d.v[ i ] + pdata[ i + 1 ];
  vmh_set_rgbdata( vmh_getv( CH_RED, newv[ CH_RED ], 0 ),
                   vmh_getv( CH_GREEN, newv[ CH_GREEN ], 0 ),
                   vmh_getv( CH_BLUE, newv[ CH_BLUE ], 0 ),
                   1 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_jmp( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  vm_d.pc = vmh_data_to_u32( pdata );
  return LEDVM_ERR_OK;
}

static int vm_h_setmin( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  memcpy( vm_d.min, pdata + 1, 3 );
  vmh_set_crtrgb( 0 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_on( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  memcpy( vm_d.on, pdata + 1, 3 );
  vmh_set_crtrgb( 0 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static int vm_h_setmax( const u8 *pdata )
{
  vmh_check_mask( pdata[ 0 ] );
  memcpy( vm_d.max, pdata + 1, 3 );
  vmh_set_crtrgb( 0 );
  VM_INC_PC;
  return LEDVM_ERR_OK;
}

static p_handler vm_handlers[] = { vm_h_nop, vm_h_setrgb, vm_h_delay, vm_h_seta,
  vm_h_setb, vm_h_repeat, vm_h_rgbdelay, vm_h_nextrgb, 
  vm_h_call, vm_h_ret, vm_h_addrgb, vm_h_jmp, 
  vm_h_setmin, vm_h_on, vm_h_setmax };

// ****************************************************************************
// Public interface

void ledvm_init()
{
  memset( &vm_d, 0, sizeof( vm_d ) );
  memset( vm_d.on, 1, 3 );
  memset( vm_d.m, 1, 3 );
  memset( vm_d.max, 32, 3 );
}

u16 ledvm_get_pc()
{
  return vm_d.pc;
}

int ledvm_run()
{
  u8 inst[ 4 ];
  int res = ledvm_ll_getinst( vm_d.pc, inst );
  u8 code = inst[ 0 ] >> 4;
  
  if( res == 0 )
    return LEDVM_ERR_INVALID_PC;
  if( code  < sizeof( vm_handlers ) / sizeof( p_handler ) )
    return vm_handlers[ code ]( inst );
  else
    return LEDVM_ERR_INVALID_OPCODE;
}

