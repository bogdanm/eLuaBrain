/*******************************************************************************
*     File:   24C16.c
*    Title:   complete TWI example code for a 24C16 EEPROM
* also needs:   lcd.h
*   Compiler:   AVR-GCC 3.3.1
*    Version:   1.0
*  last update:   30-03-2004
*   Target:   ATmega8
*   Author:   Christoph Redecker
*    Company:   www.avrbeginners.net
*******************************************************************************/
/* Modified by BogdanM as part of the 'lights' project                          *
*******************************************************************************/

#include "machine.h"
#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include "i2cee.h"

/*******************************************************************************
* TWI_init: sets bitrate in TWBR and prescaler in TWSR
* TWI_action: writes command to TWCR and takes care of TWINT and TWEN setting
* waits for TWI action to be completed and returns status code
* TWI_start: generates start condition and returns status code
* TWI_stop: generate stop condition and returns status code
* TWI_write_data: writes data to TWI bus and returns status code
* TWI_read_data: reads data from bus to TWDR and returns status code.
* if put_ack > 0, an ACK will be sent after the data has been received.
* ACKed its address before. Loops forever if slave not present!
* EE_read_data: reads one data byte from a given address
* returns different error codes (zero if success) and data in TWDR
* EE_write_data: write one data byte to a given address
* returns different error codes (zero if success)
*******************************************************************************/
static void TWI_init(char bitrate, char prescaler);
static char TWI_action(char command);
static char TWI_start(void);
static void TWI_stop(void);
static char TWI_write_data(char data);
static char TWI_read_data(char put_ack);

#define EE_ADDR 0xA0

/******************************************************************************/  
void i2cee_init (void)
{
  //init LCD
  //LCD_init();
  //set TWBR = 2 for ~400kHz SCL
  TWI_init(2, 0);
}

/******************************************************************************/
static void TWI_init(char bitrate, char prescaler)
//sets bitrate and prescaler
{
  TWBR = bitrate;
  //mask off the high prescaler bits (we only need the lower three bits)
  TWSR = prescaler & 0x03;
}

/******************************************************************************/
static char TWI_action(char command)
//starts any TWI operation (not stop), waits for completion and returns the status code
//TWINT and TWEN are set by this function, so for a simple data transfer just use TWI_action(0);
{ //make sure command is good
  TWCR = (command|(1<<TWINT)|(1<<TWEN));
  //wait for TWINT to be set after operation has been completed
  while(!(TWCR & (1<<TWINT)));
  //return status code with prescaler bits masked to zero
  return (TWSR & 0xF8);
}

/******************************************************************************/
static char TWI_start(void)
//uses TWI_action to generate a start condition, returns status code
{ //TWI_action writes the following command to TWCR: (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
  return TWI_action(1<<TWSTA);
  //return values should be 0x08 (start) or 0x10 (repeated start)
}

/******************************************************************************/
static void TWI_stop(void)
//generates stop condition
{ //as TWINT is not set after a stop condition, we can't use TWI_action here!
  TWCR = ((1<<TWINT)|(1<<TWSTO)|(1<<TWEN));
  //status code returned is 0xF8 (no specific operation)
}

/******************************************************************************/
static char TWI_write_data(char data)
//loads data into TWDR and transfers it. Works for slave addresses and normal data
//waits for completion and returns the status code.
{ //just write data to TWDR and transmit it
  TWDR = data;
  //we don't need any special bits in TWCR, just TWINT and TWEN. These are set by TWI_action.
  return TWI_action(0);
  //status code returned should be:
  //0x18 (slave ACKed address)
  //0x20 (no ACK after address)
  //0x28 (data ACKed by slave)
  //0x30 (no ACK after data transfer)
  //0x38 (lost arbitration)
  //0x40 (SLA+R transmitted, ACK received)
}

/******************************************************************************/
static char TWI_read_data(char put_ack)
{ //if an ACK is to returned to the transmitting device, set the TWEA bit
  if(put_ack)
    return(TWI_action(1<<TWEA));
  //if no ACK (a NACK) has to be returned, just receive the data
  else
    return(TWI_action(0));
  //status codes returned:
  //0x38 (lost arbitration)
  //0x40 (slave ACKed address)
  //0x48 (no ACK after slave address)
  //0x50 (AVR ACKed data)
  //0x58 (no ACK after data transfer)
}

u16 i2cee_write_block( u16 address, const u8 *pdata, u16 size )
{
  char dummy;
  u16 i;

  if( size == 0 )
    return 0;
  //we need this for the first if()
  dummy = TWI_start();
  //if the start was successful, continue, otherwise return 1
  if( ( dummy != 0x08 ) && ( dummy != 0x10 ) )
    return 0;
  // Send device address
  if( TWI_write_data( EE_ADDR ) != 0x18 )
    return 0;
  // Send high and low address 
  if( TWI_write_data( HI8( address ) ) != 0x28 )
    return 0;
  if( TWI_write_data( LO8( address ) ) != 0x28 )
    return 0;
  //now send the data bytes
  for( i = 0; i < size; i ++ )
    if( TWI_write_data( pdata[ i ] ) != 0x28 )
      break;
  TWI_stop();
  return i;
}

u16 i2cee_write_byte( u16 address, u8 data )
{
  return i2cee_write_block( address, &data, 1 );
}

u16 i2cee_read_block( u16 address, u8 *pdata, u16 size )
{
  char dummy;
  u16 i;

  if( size == 0 )
    return 0;
  //we need this for the first if()
  dummy = TWI_start();
  //as in EE_write_byte, first send the page address and the word address
  if( ( dummy != 0x08 ) && ( dummy != 0x10 ) )
    return 0;
  // Now send device address with 'write' and address bytes
  if( TWI_write_data( EE_ADDR ) != 0x18 )
    return 0;
  // Send high and low address 
  if( TWI_write_data( HI8( address ) ) != 0x28 )
    return 0;
  if( TWI_write_data( LO8( address ) ) != 0x28 )
    return 0;
  // Send 'start' again
  dummy = TWI_start();
  if( ( dummy != 0x08 ) && ( dummy != 0x10 ) )
    return 0;
  // Now send device address with 'read'
  if( TWI_write_data( EE_ADDR | 1 ) != 0x40 )
    return 0;
  // Read data
  for( i = 0; i < size; i ++ )
  {
    dummy = i == size - 1 ? 0x58 : 0x50;
    if( TWI_read_data( i != size - 1 ) != dummy )
      break;
    pdata[ i ] = TWDR;
  }
  TWI_stop();
  return i;
}

int i2cee_read_byte( u16 address )
{
  u8 data;

  return i2cee_read_block( address, &data, 1 ) == 0 ? -1 : data;
}

int i2cee_is_write_finished()
{
  int res;

  TWI_start();
  res = TWI_write_data( EE_ADDR ) == 0x18;
  if( res )
    TWI_stop();
  return res;
}

