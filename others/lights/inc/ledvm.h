// LED virtual machine 

#ifndef __LEDVM_H__
#define __LEDVM_H_

#include "type.h"

#define LEDVM_REP_COUNTERS    8
#define LEDVM_MAX_VAL         32
#define LEDVM_MAX_CALLS       8
#define LEDVM_EMPTY_MARK      0xAA55AA
#define LEDVM_NUM_SAME        0xFE
#define LEDVM_NUM_RAND        0xFF
#define LEDVM_MASKNEW_BIT     0x08
#define LEDVM_MASKR_BIT       0x04
#define LEDVM_MASKG_BIT       0x02
#define LEDVM_MASKB_BIT       0x01
#define LEDVM_CNT_LIM         0

// Errors
enum
{
  LEDVM_ERR_OK = 0,
  LEDVM_ERR_INVALID_OPCODE,
  LEDVM_ERR_INVALID_INSTRUCTION,
  LEDVM_ERR_TOO_MANY_CALLS,
  LEDVM_ERR_INVALID_RET,
  LEDVM_ERR_INVALID_PC
};

void ledvm_init();
int ledvm_run();
u16 ledvm_get_pc();

// Low-level interface (to be implemented by host)
void ledvm_ll_setrgb( s8 r, s8 g, s8 b );
void ledvm_ll_delayms( u32 ms );
int ledvm_ll_getinst( u16 pc, u8 *pinst );
int ledvm_ll_rand();

#endif // #ifndef __LEDVM_H_

