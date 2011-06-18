// Original enc28j60 code bt Iain Derrington
// Modified by BogdanM for eLua

#include "platform_conf.h"
#ifdef BUILD_ENC28J60

#include "enc28j60.h"
#include "platform.h"
#include <stdio.h>

/******************************************************************************/
/** \file enc28j60.c
 *  \brief Driver code for enc28j60.
 *  \author Iain Derrington (www.kandi-electronics.com)
 *  \date  0.1 20/06/07 First Draft \n
 *         0.2 11/07/07 Removed CS check macros. Fixed bug in writePhy
 *  
 */
/*******************************************************************************/


// define private variables

/** MAC address. Will move this some where else*/
TXSTATUS TxStatus;

// define private functions
static u08 ReadETHReg(u08);         // read a ETX reg
static u08 ReadMacReg(u08);         // read a MAC reg
static u16 ReadPhyReg(u08);         // read a PHY reg
static u16 ReadMacBuffer(u08 * ,u08);    //read the mac buffer (ptrBuffer, no. of bytes)
static u08 WriteCtrReg(u08,u08);               // write to control reg
static u8 ReadCtrReg(u8,int);           // read a control reg
static u08 WritePhyReg(u08,u16);               // write to a phy reg
static u16 WriteMacBuffer(u08 *,u16);    // write to mac buffer
static void ResetMac(void);

static u08 SetBitField(u08, u08);
static u08 ClrBitField(u08, u08);
static void BankSel(u08);

//define usefull macros

/** MACRO for selecting or deselecting chip select for the ENC28J60. Some HW dependancy.*/
#define SEL_MAC(x) platform_pio_op( ENC28J60_CS_PORT, 1 << ENC28J60_CS_PIN, x == TRUE ? PLATFORM_IO_PIN_CLEAR : PLATFORM_IO_PIN_SET )
/** MACRO for rev B5 fix.*/
#define ERRATAFIX   do { SetBitField(ECON1, ECON1_TXRST);ClrBitField(ECON1, ECON1_TXRST);ClrBitField(EIR, EIR_TXERIF | EIR_TXIF); } while(0)

static u16 SPIWrite( u8 *pdata, u16 size )
{
  u16 i;

  for( i = 0; i < size; i ++ )
    platform_spi_send_recv( ENC28J60_SPI_ID, pdata[ i ] );
  return size;
}

static u16 SPIRead( u8 *pdata, u16 size )
{
  u16 i;

  for( i = 0; i < size; i ++ )
    pdata[ i ] = platform_spi_send_recv( ENC28J60_SPI_ID, 0xFF );
  return size;
}

static const u8 *pdata;
static u32 theclock;

/***********************************************************************/
/** \brief Initialise the MAC.
 *
 * Description: \n
 * a) Setup SPI device. Assume Reb B5 for sub 8MHz operation \n
 * b) Setup buffer ptrs to devide memory in In and Out mem \n
 * c) Setup receive filters (accept only unicast).\n
 * d) Setup MACON registers (MAC control registers)\n
 * e) Setup MAC address
 * f) Setup Phy registers
 * \author Iain Derrington (www.kandi-electronics.com)
 */
/**********************************************************************/
void initMAC( const u8* bytMacAddress )
{
  pdata = bytMacAddress;
  // Initialize the SPI and the CS pin
  theclock = platform_spi_setup( ENC28J60_SPI_ID, PLATFORM_SPI_MASTER, ENC28J60_SPI_CLOCK, 0, 0, 8 );
  platform_pio_op( ENC28J60_CS_PORT, 1 << ENC28J60_CS_PIN, PLATFORM_IO_PIN_SET );
  platform_pio_op( ENC28J60_CS_PORT, 1 << ENC28J60_CS_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
#if defined( ENC28J60_RESET_PORT ) && defined( ENC28J60_RESET_PIN )
  platform_pio_op( ENC28J60_RESET_PORT, 1 << ENC28J60_RESET_PIN, PLATFORM_IO_PIN_CLEAR );
  platform_pio_op( ENC28J60_RESET_PORT, 1 << ENC28J60_RESET_PIN, PLATFORM_IO_PIN_DIR_OUTPUT );
  platform_timer_delay( 0, 100000 );
  platform_pio_op( ENC28J60_RESET_PORT, 1 << ENC28J60_RESET_PIN, PLATFORM_IO_PIN_SET );
#endif

  ResetMac();       // erm. Resets the MAC.
                    // setup memory by defining ERXST and ERXND
  BankSel(0);       // select bank 0
  WriteCtrReg(ERXSTL,(u08)( RXSTART & 0x00ff));    
  WriteCtrReg(ERXSTH,(u08)((RXSTART & 0xff00)>> 8));
  WriteCtrReg(ERXNDL,(u08)( RXEND   & 0x00ff));
  WriteCtrReg(ERXNDH,(u08)((RXEND   & 0xff00)>>8));
                    // Make sure Rx Read ptr is at the start of Rx segment
  WriteCtrReg(ERXRDPTL, (u08)( RXSTART & 0x00ff));
  WriteCtrReg(ERXRDPTH, (u08)((RXSTART & 0xff00)>> 8));
  BankSel(1);                             // select bank 1
  WriteCtrReg(ERXFCON,( ERXFCON_UCEN + ERXFCON_CRCEN + ERXFCON_BCEN));

                // Initialise the MAC registers
  BankSel(2);                             // select bank 2
  SetBitField(MACON1, MACON1_MARXEN);     // Enable reception of frames
  WriteCtrReg(MACLCON2, 63);
  WriteCtrReg(MACON3, MACON3_FRMLNEN +    // Type / len field will be checked
                      MACON3_TXCRCEN +    // MAC will append valid CRC
                      MACON3_PADCFG0);    // All small packets will be padded
                      

  SetBitField(MACON4, MACON4_DEFER);      
  WriteCtrReg(MAMXFLL, (u08)( MAXFRAMELEN & 0x00ff));     // set max frame len
  WriteCtrReg(MAMXFLH, (u08)((MAXFRAMELEN & 0xff00)>>8));
  WriteCtrReg(MABBIPG, 0x12);             // back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAIPGL , 0x12);             // non back to back interpacket gap. set as per data sheet
  WriteCtrReg(MAIPGH , 0x0C);

  
            //Program our MAC address
  BankSel(3);              
  WriteCtrReg(MAADR1,bytMacAddress[0]);   
  WriteCtrReg(MAADR2,bytMacAddress[1]);  
  WriteCtrReg(MAADR3,bytMacAddress[2]);
  WriteCtrReg(MAADR4,bytMacAddress[3]);
  WriteCtrReg(MAADR5,bytMacAddress[4]);
  WriteCtrReg(MAADR6,bytMacAddress[5]);

  // Initialise the PHY registes
  WritePhyReg(PHCON1, 0x000);
  WritePhyReg(PHCON2, PHCON2_HDLDIS);
  WriteCtrReg(ECON1,  ECON1_RXEN);     //Enable the chip for reception of packets
}

void testMe()
{
  unsigned i;

  printf( "***********************\n" );
  printf( "Clock: %u\n", theclock );
  for( i = 0; i < 6; i ++ )
    printf( "%02X ", pdata[ i ] );
  printf( "\n" );
  BankSel( 3 );
  printf( "%02X %02X %02X %02X %02X %02X\n", ReadMacReg( MAADR1 ),  ReadMacReg( MAADR2 ), ReadMacReg( MAADR3 ), 
    ReadMacReg( MAADR4 ), ReadMacReg( MAADR5 ), ReadMacReg( MAADR6 ) );
  printf( "\n" );
  printf( "Rev=%d\n", ReadETHReg( EREVID ) );
  printf( "***********************\n" );
}


/***********************************************************************/
/** \brief Writes a packet to the ENC28J60.
 *
 * Description: Writes ui_len bytes of data from ptrBufffer into ENC28J60.
 *              puts the necessary padding around the packet to make it a legit
                MAC packet.\n \n 
                1) Program ETXST.   \n
                2) Write per packet control byte.\n
                3) Program ETXND.\n
                4) Set ECON1.TXRTS.\n
                5) Check ESTAT.TXABRT. \n

 * \author Iain Derrington (www.kandi-electronics.com)
 * \param ptrBuffer ptr to byte buffer. 
 * \param ui_Len Number of bytes to write from buffer. 
 * \return uint True or false. 
 */
/**********************************************************************/
u16 MACWrite(u08 * ptrBuffer, u16 ui_Len)
{
  volatile u16 address = TXSTART;
  u08  bytControl;
  
  bytControl = 0x00;

  
  BankSel(0);                                          // select bank 0
  WriteCtrReg(ETXSTL,(u08)( TXSTART & 0x00ff));        // write ptr to start of Tx packet
  WriteCtrReg(ETXSTH,(u08)((TXSTART & 0xff00)>>8));

  WriteCtrReg(EWRPTL,(u08)( TXSTART & 0x00ff));        // Set write buffer to point to start of Tx Buffer
  WriteCtrReg(EWRPTH,(u08)((TXSTART & 0xff00)>>8));

  WriteMacBuffer(&bytControl,1);                       // write per packet control byte
  address++;
  
  address+=WriteMacBuffer(ptrBuffer, ui_Len);          // write packet. Assume correct formating src, dst, type  + data

 WriteCtrReg(ETXNDL, (u08)( address & 0x00ff));       // Tell MAC when the end of the packet is
  WriteCtrReg(ETXNDH, (u08)((address & 0xff00)>>8));
  
 
  ClrBitField(EIR,EIR_TXIF);
  //SetBitField(EIE, EIE_TXIE |EIE_INTIE);

  ERRATAFIX;    
  SetBitField(ECON1, ECON1_TXRTS);                     // begin transmitting;

  do
  {
    // [TODO] add timers here or make packet sending completely asynchronous
  }while (!(ReadETHReg(EIR) & (EIR_TXIF)));             // kill some time. Note: Nice place to block?

  ClrBitField(ECON1, ECON1_TXRTS);
  
  BankSel(0);                       //read tx status bytes
  address++;                                          // increment ptr to address to start of status struc
  WriteCtrReg(ERDPTL, (u08)( address & 0x00ff));      // Setup the buffer read ptr to read status struc
  WriteCtrReg(ERDPTH, (u08)((address & 0xff00)>>8));
  ReadMacBuffer(&TxStatus.v[0],7);

  if (ReadETHReg(ESTAT) & ESTAT_TXABRT)                // did transmission get interrupted?
  {
    if (TxStatus.bits.LateCollision)
    {
      ClrBitField(ECON1, ECON1_TXRTS);
      SetBitField(ECON1, ECON1_TXRTS);
      
      ClrBitField(ESTAT,ESTAT_TXABRT | ESTAT_LATECOL);
    }
    ClrBitField(EIR, EIR_TXERIF | EIR_TXIF);
    ClrBitField(ESTAT,ESTAT_TXABRT);

    return FALSE;                                          // packet transmit failed. Inform calling function
  }                                                        // calling function may inquire why packet failed by calling [TO DO] function
  else
  {
    return TRUE;                                           // all fan dabby dozy
  }
}

/***********************************************************************/
/** \brief Tries to read a packet from the ENC28J60.
 *
 * Description: If a valid packet is available in the ENC28J60 this function reads the packet into a
 *              buffer. The memory within the ENC28J60 will then be released. This version of the
                driver does not use interrupts so this function needs to be polled.\n \n
 * 
 * 1) Read packet count register. If >0 then continue else return. \n
 * 2) Read the current ERXRDPTR value. \n           
 * 3) Write this value into ERDPT.     \n
 * 4) First two bytes contain the ptr to the start of next packet. Read this value in. \n
 * 5) Calculate length of packet. \n
 * 6) Read in status byte into private variable. \n
 * 7) Read in packet and place into buffer. \n
 * 8) Free up memory in the ENC. \n
 *
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param ptrBuffer ptr to buffer of bytes where the packet should be read into. 
 * param maxsize maximum number of bytes to read
 * \return the number of bytes actually read
 */
/**********************************************************************/
u16 MACRead(u08 * ptrBuffer, u16 maxsize )
{
  static u16 nextpckptr = RXSTART;
  RXSTATUS ptrRxStatus;
  u08 bytPacket;
  u16 actsize;

  BankSel(1);
  
  bytPacket = ReadETHReg(EPKTCNT);          // How many packets have been received
  
  if(bytPacket == 0)
    return bytPacket;                       // No full packets received
  
  BankSel(0);

  WriteCtrReg(ERDPTL,(u08)(nextpckptr & 0x00ff));   //write this value to read buffer ptr
  WriteCtrReg(ERDPTH,(u08)((nextpckptr & 0xff00)>>8));

  ReadMacBuffer((u08*)&ptrRxStatus.v[0],6);             // read next packet ptr + 4 status bytes
  nextpckptr = ptrRxStatus.bits.NextPacket;
      
  actsize = ptrRxStatus.bits.ByteCount;
  if( actsize > maxsize )
    actsize = maxsize;
  ReadMacBuffer(ptrBuffer,actsize);            // read packet into buffer
                                        // ptrBuffer should now contain a MAC packet
    BankSel(0);
  WriteCtrReg(ERXRDPTL,ptrRxStatus.v[0]);  // free up ENC memory my adjustng the Rx Read ptr
  WriteCtrReg(ERXRDPTH,ptrRxStatus.v[1]);
 
  // decrement packet counter
  SetBitField(ECON2, ECON2_PKTDEC);


  return actsize;
}

/*------------------------Private Functions-----------------------------*/

/***********************************************************************/
/** \brief ReadETHReg.
 *
 * Description: Reads contents of the addressed ETH reg over SPI bus. Assumes correct bank selected.
 *              
 *              
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytAddress Address of register to be read
 * \return byte Value of register.
 */
/**********************************************************************/
static u08 ReadETHReg(u08 bytAddress)
{
  u08 bytData;
  
  if (bytAddress > 0x1F)    
    return FALSE;                 // address invalid, [TO DO] 

  SEL_MAC(TRUE);                 // ENC CS low
  SPIWrite(&bytAddress,1);       // write opcode
  SPIRead(&bytData, 1);          // read value
  SEL_MAC(FALSE);

  return bytData;

}

/***********************************************************************/
/** \brief ReadMacReg.
 *
 * Description: Read contents of addressed MAC register over SPI bus. Assumes correct bank selected.
 *                    
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytAddress Address of register to read.
 * \return byte Contens of register just read.
 */
/**********************************************************************/
static u08 ReadMacReg(u08 bytAddress)
{
  u08 bytData;

  if (bytAddress > 0x1F)    
    return FALSE;                 // address invalid, [TO DO] 

  SEL_MAC(TRUE);                 // ENC CS low
  SPIWrite(&bytAddress,1);    // write opcode
  SPIRead(&bytData, 1);       // read dummy byte
  SPIRead(&bytData, 1);       // read value
  SEL_MAC(FALSE);

  return bytData;
}

/***********************************************************************/
/** \brief Write to phy Reg. 
 *
 * Description:  Writing to PHY registers is different to writing the other regeisters in that
 *               the registers can not be accessed directly. This function wraps up the requirements
 *               for writing to the PHY reg. \n \n
 *                
 *               1) Write address of phy reg to MIREGADR. \n
 *               2) Write lower 8 bits of data to MIWRL. \n
 *               3) Write upper 8 bits of data to MIWRL. \n \n            
 *              
 *              
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param address
 * \param data
 * \return byte
 */
/**********************************************************************/
static u08 WritePhyReg(u08 address, u16 data)
{ 
  if (address > 0x14)
    return FALSE;
  
  BankSel(2);

  WriteCtrReg(MIREGADR,address);        // Write address of Phy reg 
  WriteCtrReg(MIWRL,(u08)data);        // lower phy data 
  WriteCtrReg(MIWRH,((u08)(data >>8)));    // Upper phydata

  return TRUE;
}

/***********************************************************************/
/** \brief Read from PHY reg.
 *
 * Description: No direct access allowed to phy registers so the folling process must take place. \n \n
 *              1) Write address of phy reg to read from into MIREGADR. \n
 *              2) Set MICMD.MIIRD bit and wait 10.4uS. \n
 *              3) Clear MICMD.MIIRD bit. \n
 *              4) Read data from MIRDL and MIRDH reg. \n
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param address
 */
/**********************************************************************/
static u16 ReadPhyReg(u08 address)
{
 u16 uiData;
 u08 bytStat;

  BankSel(2);
  WriteCtrReg(MIREGADR,address);    // Write address of phy register to read
  SetBitField(MICMD, MICMD_MIIRD);  // Set rd bit
  do                                  
  {
    bytStat = ReadMacReg(MISTAT);
  }while(bytStat & MISTAT_BUSY);

  ClrBitField(MICMD,MICMD_MIIRD);   // Clear rd bit
  uiData = (u16)ReadMacReg(MIRDL);       // Read low data byte
  uiData |=((u16)ReadMacReg(MIRDH)<<8); // Read high data byte

  return uiData;
}

/***********************************************************************/
/** \brief Write to a control reg .
 *
 * Description: Writes a byte to the address register. Assumes that correct bank has
 *              all ready been selected
 *              
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytAddress Address of register to be written to. 
 * \param bytData    Data to be written. 
 * \returns byte  
 */
/**********************************************************************/
static u08 WriteCtrReg(u08 bytAddress,u08 bytData)
{
  if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= WCR_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);

  return TRUE;
}

/***********************************************************************/
/** \brief Read bytes from MAC data buffer.
 *
 * Description: Reads a number of bytes from the ENC28J60 internal memory. Assumes auto increment
 *              is on. 
 *              
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytBuffer  Buffer to place data in. 
 * \param byt_length Number of bytes to read.
 * \return uint  Number of bytes read.
 */
/**********************************************************************/
static u16 ReadMacBuffer(u08 * bytBuffer,u08 byt_length)
{
  u08 bytOpcode;
  u16 len;

  bytOpcode = RBM_OP;
  SEL_MAC(TRUE);            // ENC CS low
  SPIWrite(&bytOpcode,1);   // Tx opcode 
  len = SPIRead(bytBuffer, byt_length);   // read bytes into buffer
  SEL_MAC(FALSE);           // release CS

  return len;

}
/***********************************************************************/
/** \brief Write bytes to MAC data buffer.[UNTESTED]
 *
 * Description: Reads a number of bytes from the ENC28J60 internal memory. Assumes auto increment
 *              is on.
 *             
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytBuffer
 * \param ui_len
 * \return uint
 */
/**********************************************************************/
static u16 WriteMacBuffer(u08 * bytBuffer,u16 ui_len)
{
  u08 bytOpcode;
  u16 len;

  bytOpcode = WBM_OP;
  SEL_MAC(TRUE);            // ENC CS low
  SPIWrite(&bytOpcode,1);   // Tx opcode 
  len = SPIWrite(bytBuffer, ui_len);   // read bytes into buffer
  SEL_MAC(FALSE);           // release CS

  return len;

}

/***********************************************************************/
/** \brief Set bit field. 
 *
 * Description: Sets the bit/s at the address register. Assumes correct bank has been selected.
 *                           
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false
 */
/**********************************************************************/
static u08 SetBitField(u08 bytAddress, u08 bytData)
{
  if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= BFS_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);

  return TRUE;
}

/***********************************************************************/
/** \brief Clear bit field on ctr registers.
 *
 * Description: Sets the bit/s at the address register. Assumes correct bank has been selected.
 *             
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bytAddress Address of registed where bit is to be set
 * \param bytData    Sets all the bits high.
 * \return byte      True or false.
 */
/**********************************************************************/
static u08 ClrBitField(u08 bytAddress, u08 bytData)
{
 if (bytAddress > 0x1f)
  {
    return FALSE;
  }

  bytAddress |= BFC_OP;       // Set opcode
  SEL_MAC(TRUE);              // ENC CS low
  SPIWrite(&bytAddress,1);    // Tx opcode and address
  SPIWrite(&bytData,1);       // Tx data
  SEL_MAC(FALSE);

  return TRUE;

}

/***********************************************************************/
/** \brief Bank Select.
 *
 * Description: Select the required bank within the ENC28J60
 *              
 *              
 * \author Iain Derrington (www.kandi-electronics.com)
 * \param bank Value between 0 and 3.
 */
/**********************************************************************/
static void BankSel(u08 bank)
{
  u08 temp;

  if (bank >3)
    return;
  
  temp = ReadETHReg(ECON1);       // Read ECON1 register
  temp &= ~ECON1_BSEL;            // mask off the BSEL bits
  temp |= bank;                   // set BSEL bits
  WriteCtrReg(ECON1, temp);       // write new values back to ENC28J60
}
/***********************************************************************/
/** \brief ResetMac.
 *
 * Description: Sends command to reset the MAC.
 *                
 * \author Iain Derrington (www.kandi-electronics.com)
 */
/**********************************************************************/
static void ResetMac(void)
{
  u08 bytOpcode = RESET_OP;
  SEL_MAC(TRUE);              // ENC CS low
  SPIWrite(&bytOpcode,1);     // Tx opcode and address
  SEL_MAC(FALSE);
}

void SetRXInterrupt( int enabled )
{
  if( enabled )
    SetBitField( EIE, EIE_PKTIE | EIE_INTIE );
  else
    ClrBitField( EIE, EIE_PKTIE | EIE_INTIE );
}

#endif // #ifdef BUILD_ENC28J60

