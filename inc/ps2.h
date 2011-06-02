// PS/2 keyboard decoder

#ifndef __PS2_H__
#define __PS2_H__

#include "type.h"

#define PS2_SHIFT             0x0001
#define PS2_CTRL              0x0002
#define PS2_ALT               0x0004
#define PS2_BASEF_MASK        ( PS2_SHIFT | PS2_CTRL | PS2_ALT )
#define PS2_BASEF_SHIFT       13
#define PS2_RAW_TO_CODE( x )  ( ( x ) & ~( PS2_BASEF_MASK << PS2_BASEF_SHIFT ) )
#define PS2_RAW_TO_MODS( x )  ( ( ( x ) >> PS2_BASEF_SHIFT ) & PS2_BASEF_MASK )

void ps2_init();
int ps2_term_translate();
int ps2_get_rawkey( s32 timeout );
int ps2_std_get( s32 timeout );
int ps2_term_get( int mode );
int ps2_term_translate( int code );

#endif

