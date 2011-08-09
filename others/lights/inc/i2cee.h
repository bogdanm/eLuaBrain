// 24LCxxx EEROM access routines

#ifndef __I2CEE_H__
#define __I2CEE_H__

#include "type.h"

void i2cee_init( void );
u16 i2cee_write_byte( u16 address, u8 data );
int i2cee_is_write_finished();
int i2cee_read_byte( u16 address );
u16 i2cee_write_block( u16 address, const u8 *pdata, u16 size );
u16 i2cee_read_block( u16 address, u8 *pdata, u16 size );

#endif // #ifndef __I2CEE_H__

