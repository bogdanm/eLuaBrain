#ifndef ENC28J60_H
#define ENC28J60_H

#include "type.h"

typedef u8 u08;

//initialisation routine
void initMAC( const u8* bytMacAddress );
// function to write and send a packet from the ENC28J60_H
u16 MACWrite(u08*, u16);
// function to read a byte (if there) into a buffer
u08 MACRead(u08*);

// useful structures
typedef union {
	u08 v[7];
	struct {
		u16	ByteCount;
		u08	CollisionCount:4;
		u08	CRCError:1;
		u08	LengthCheckError:1;
		u08	LengthOutOfRange:1;
		u08	Done:1;
		u08	Multicast:1;
		u08	Broadcast:1;
		u08	PacketDefer:1;
		u08	ExcessiveDefer:1;
		u08	MaximumCollisions:1;
		u08	LateCollision:1;
		u08	Giant:1;
		u08	Underrun:1;
		u16	BytesTransmittedOnWire;
		u08	ControlFrame:1;
		u08	PAUSEControlFrame:1;
		u08	BackpressureApplied:1;
		u08	VLANTaggedFrame:1;
		u08	Zeros:4;
	} bits;
} TXSTATUS;

typedef union {
	u08 v[6];
	struct {
		u16     NextPacket;
                u16	ByteCount;
		u08	LongEvent:1;
		u08	Reserved:1;
		u08	CarrierEvent:1;
		u08	Reserved2:1;
		u08	CRCError:1;
		u08	LenChkError:1;
		u08	LenOutofRange:1;
		u08	RxOk:1;
		u08	RxMultiCast:1;
		u08	RxBroadCast:1;
		u08	DribbleNibble:1;
		u08	RxCntrlFrame:1;
		u08	RxPauseFrame:1;
		u08	RxUkwnOpcode:1;
                u08     RxVlan:1;
		u08	Zeros:1;
	} bits;
} RXSTATUS;


// constants used to set up memory
#define RXSTART        0x0000
#define RXEND          0x0fff
#define TXSTART        0x1000
#define TXEND          0x1fff
#define RXMAXBUFLEN    RXEND - RXSTART
#define TXMAXBUFLEN    TXEND - TXSTART

#define MAXFRAMELEN     1518

#define RBM_OP           0x3a
#define WCR_OP           0x40
#define WBM_OP           0x7a
#define BFS_OP           0x80
#define BFC_OP           0xa0
#define RESET_OP         0xff

// Bank 0 registers --------
#define ERDPTL		0x00
#define ERDPTH		0x01
#define EWRPTL		0x02
#define EWRPTH		0x03
#define ETXSTL		0x04
#define ETXSTH		0x05
#define ETXNDL		0x06
#define ETXNDH		0x07
#define ERXSTL		0x08
#define ERXSTH		0x09
#define ERXNDL		0x0A
#define ERXNDH		0x0B
#define ERXRDPTL	0x0C
#define ERXRDPTH	0x0D
#define ERXWRPTL	0x0E
#define ERXWRPTH	0x0F
#define EDMASTL		0x10
#define EDMASTH		0x11
#define EDMANDL		0x12
#define EDMANDH		0x13
#define EDMADSTL	0x14
#define EDMADSTH	0x15
#define EDMACSL		0x16
#define EDMACSH		0x17
#define EIE             0x1B
#define EIR             0x1C
#define ESTAT		0x1D
#define ECON2		0x1E
#define ECON1		0x1F


#define EHT0		0x00
#define EHT1		0x01
#define EHT2		0x02
#define EHT3		0x03
#define EHT4		0x04
#define EHT5		0x05
#define EHT6		0x06
#define EHT7		0x07
#define EPMM0		0x08
#define EPMM1		0x09
#define EPMM2		0x0A
#define EPMM3		0x0B
#define EPMM4		0x0C
#define EPMM5		0x0D
#define EPMM6		0x0E
#define EPMM7		0x0F
#define EPMCSL		0x10
#define EPMCSH		0x11
#define EPMOL		0x14
#define EPMOH		0x15
#define ERXFCON		0x18
#define EPKTCNT		0x19

// Bank 2 registers -----
#define MACON1		0x00
#define MACON3		0x02
#define MACON4		0x03
#define MABBIPG		0x04
#define MAIPGL		0x06
#define MAIPGH		0x07
#define MACLCON1	0x08
#define MACLCON2	0x09
#define MAMXFLL		0x0A
#define MAMXFLH		0x0B
#define MICMD		0x12
#define MIREGADR	0x14
#define MIWRL		0x16
#define MIWRH		0x17
#define MIRDL		0x18
#define MIRDH		0x19

// Bank 3 registers -----
#define MAADR5		0x00
#define MAADR6		0x01
#define MAADR3		0x02
#define MAADR4		0x03
#define MAADR1		0x04
#define MAADR2		0x05
#define EBSTSD		0x06
#define EBSTCON		0x307
#define EBSTCSL		0x08
#define EBSTCSH		0x09
#define MISTAT		0x0A
#define EREVID		0x12
#define ECOCON		0x15
#define EFLOCON		0x17
#define EPAUSL		0x18
#define EPAUSH		0x19

/******************************************************************************
* PH Register Locations
******************************************************************************/
#define PHCON1	0x00
#define PHSTAT1	0x01
#define PHID1	0x02
#define PHID2	0x03
#define PHCON2	0x10
#define PHSTAT2	0x11
#define PHIE	0x12
#define PHIR	0x13
#define PHLCON	0x14


/******************************************************************************
* Individual Register Bits
******************************************************************************/
// ETH/MAC/MII bits

// EIE bits ----------
#define	EIE_INTIE		(1<<7)
#define	EIE_PKTIE		(1<<6)
#define	EIE_DMAIE		(1<<5)
#define	EIE_LINKIE		(1<<4)
#define	EIE_TXIE		(1<<3)
#define	EIE_TXERIE		(1<<1)
#define	EIE_RXERIE		(1)

// EIR bits ----------
#define	EIR_PKTIF		(1<<6)
#define	EIR_DMAIF		(1<<5)
#define	EIR_LINKIF		(1<<4)
#define	EIR_TXIF		(1<<3)
#define	EIR_TXERIF		(1<<1)
#define	EIR_RXERIF		(1)
	
// ESTAT bits ---------
#define	ESTAT_INT		(1<<7)
#define ESTAT_BUFER		(1<<6)
#define	ESTAT_LATECOL	(1<<4)
#define	ESTAT_RXBUSY	(1<<2)
#define	ESTAT_TXABRT	(1<<1)
#define	ESTAT_CLKRDY	(1)
	
// ECON2 bits --------
#define	ECON2_AUTOINC	(1<<7)
#define	ECON2_PKTDEC	(1<<6)
#define	ECON2_PWRSV		(1<<5)
#define	ECON2_VRPS		(1<<3)
	
// ECON1 bits --------
#define	ECON1_TXRST		(1<<7)
#define	ECON1_RXRST		(1<<6)
#define	ECON1_DMAST		(1<<5)
#define	ECON1_CSUMEN	(1<<4)
#define	ECON1_TXRTS		(1<<3)
#define	ECON1_RXEN		(1<<2)
#define	ECON1_BSEL1		(1<<1)
#define	ECON1_BSEL0		(1)
#define ECON1_BSEL              (3)
      
// ERXFCON bits ------
#define	ERXFCON_UCEN	(u08)(1<<7)
#define	ERXFCON_ANDOR	(u08)(1<<6)
#define	ERXFCON_CRCEN	(u08)(1<<5)
#define	ERXFCON_PMEN	(u08)(1<<4)
#define	ERXFCON_MPEN	(u08)(1<<3)
#define	ERXFCON_HTEN	(u08)(1<<2)
#define	ERXFCON_MCEN	(u08)(1<<1)
#define	ERXFCON_BCEN	(u08)(1)
	
// MACON1 bits --------
#define	MACON1_TXPAUS	(1<<3)
#define	MACON1_RXPAUS	(1<<2)
#define	MACON1_PASSALL	(1<<1)
#define	MACON1_MARXEN	(u08)(1)
	
// MACON3 bits --------
#define	MACON3_PADCFG2	(1<<7)
#define	MACON3_PADCFG1	(1<<6)
#define	MACON3_PADCFG0	(1<<5)
#define	MACON3_TXCRCEN	(1<<4)
#define	MACON3_PHDREN	(1<<3)
#define	MACON3_HFRMEN	(1<<2)
#define	MACON3_FRMLNEN	(1<<1)
#define	MACON3_FULDPX	(1)
	
// MACON4 bits --------
#define	MACON4_DEFER	(1<<6)
#define	MACON4_BPEN		(1<<5)
#define	MACON4_NOBKOFF	(1<<4)
	
// MICMD bits ---------
#define	MICMD_MIISCAN	(1<<1)
#define	MICMD_MIIRD		(1)

// EBSTCON bits -----
#define	EBSTCON_PSV2	(1<<7)
#define	EBSTCON_PSV1	(1<<6)
#define	EBSTCON_PSV0	(1<<5)
#define	EBSTCON_PSEL	(1<<4)
#define	EBSTCON_TMSEL1	(1<<3)
#define	EBSTCON_TMSEL0	(1<<2)
#define	EBSTCON_TME		(1<<1)
#define	EBSTCON_BISTST	(1)

// MISTAT bits --------
#define	MISTAT_NVALID	(1<<2)
#define	MISTAT_SCAN		(1<<1)
#define	MISTAT_BUSY		(1)
	
// ECOCON bits -------
#define	ECOCON_COCON2	(1<<2)
#define	ECOCON_COCON1	(1<<1)
#define	ECOCON_COCON0	(1)
	
// EFLOCON bits -----
#define	EFLOCON_FULDPXS	(1<<2)
#define	EFLOCON_FCEN1	(1<<1)
#define	EFLOCON_FCEN0	(1)



// PHY bits

// PHCON1 bits ----------
#define	PHCON1_PRST	(1ul<<15)
#define	PHCON1_PLOOPBK	(1ul<<14)
#define	PHCON1_PPWRSV	(1ul<<11)
#define	PHCON1_PDPXMD	(1ul<<8)

// PHSTAT1 bits --------
#define	PHSTAT1_PFDPX	(1ul<<12)
#define	PHSTAT1_PHDPX	(1ul<<11)
#define	PHSTAT1_LLSTAT	(1ul<<2)
#define	PHSTAT1_JBSTAT	(1ul<<1)

// PHID2 bits --------
#define	PHID2_PID24		(1ul<<15)
#define	PHID2_PID23		(1ul<<14)
#define	PHID2_PID22		(1ul<<13)
#define	PHID2_PID21		(1ul<<12)
#define	PHID2_PID20		(1ul<<11)
#define	PHID2_PID19		(1ul<<10)
#define	PHID2_PPN5		(1ul<<9)
#define	PHID2_PPN4		(1ul<<8)
#define	PHID2_PPN3		(1ul<<7)
#define	PHID2_PPN2		(1ul<<6)
#define	PHID2_PPN1		(1ul<<5)
#define	PHID2_PPN0		(1ul<<4)
#define	PHID2_PREV3		(1ul<<3)
#define	PHID2_PREV2		(1ul<<2)
#define	PHID2_PREV1		(1ul<<1)
#define	PHID2_PREV0		(1ul)

// PHCON2 bits ----------
#define	PHCON2_FRCLNK	(1ul<<14)
#define	PHCON2_TXDIS	(1ul<<13)
#define	PHCON2_JABBER	(1ul<<10)
#define	PHCON2_HDLDIS	(1ul<<8)

// PHSTAT2 bits --------
#define	PHSTAT2_TXSTAT	(1ul<<13)
#define	PHSTAT2_RXSTAT	(1ul<<12)
#define	PHSTAT2_COLSTAT	(1ul<<11)
#define	PHSTAT2_LSTAT	(1ul<<10)
#define	PHSTAT2_DPXSTAT	(1ul<<9)
#define	PHSTAT2_PLRITY	(1ul<<5)

// PHIE bits -----------
#define	PHIE_PLNKIE		(1ul<<4)
#define	PHIE_PGEIE		(1ul<<1)

// PHIR bits -----------
#define	PHIR_PLNKIF		(1ul<<4)
#define	PHIR_PGIF		(1ul<<2)

// PHLCON bits -------
#define	PHLCON_LACFG3	(1ul<<11)
#define	PHLCON_LACFG2	(1ul<<10)
#define	PHLCON_LACFG1	(1ul<<9)
#define	PHLCON_LACFG0	(1ul<<8)
#define	PHLCON_LBCFG3	(1ul<<7)
#define	PHLCON_LBCFG2	(1ul<<6)
#define	PHLCON_LBCFG1	(1ul<<5)
#define	PHLCON_LBCFG0	(1ul<<4)
#define	PHLCON_LFRQ1	(1ul<<3)
#define	PHLCON_LFRQ0	(1ul<<2)
#define	PHLCON_STRCH	(1ul<<1)


#endif

