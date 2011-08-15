/*-----------------------------------------------------------------------*/
/* MMC/SDSC/SDHC (in SPI mode) control module for STM32 Version 1.1.6    */
/* (C) Martin Thomas, 2010 - based on the AVR MMC module (C)ChaN, 2007   */
/*-----------------------------------------------------------------------*/

/* Copyright (c) 2010, Martin Thomas, ChaN
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

#include "platform_conf.h"
#if defined( BUILD_MMCFS ) && defined( MMCFS_SDIO_STM32 )

#include "stm32f10x.h"
#include "ffconf.h"
#include "diskio.h"

#include "sdcard.h"
#include "stm32f10x_sdio.h"
#include "nand_if.h"
#include "fsmc_nand.h"
#include <stdio.h>

static SD_CardInfo SDCardInfo2;

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv		/* Physical drive number (0) */
)
{
  if( drv == 0 )
  {
    SD_Init();
    SD_InitializeCards();
    SD_GetCardInfo(&SDCardInfo2);
    SD_SelectDeselect((uint32_t) (SDCardInfo2.RCA << 16));
    SD_SetDeviceMode(SD_DMA_MODE);
    NVIC_InitTypeDef nvic_init_structure;
    nvic_init_structure.NVIC_IRQChannel = SDIO_IRQn;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 4;
    nvic_init_structure.NVIC_IRQChannelSubPriority = 0;
    nvic_init_structure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_structure);
  } 
  else 
  {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
  	NAND_Init();
  }
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv		/* Physical drive number (0) */
)
{
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,			/* Physical drive number (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
  while( count )
  {
    if( drv == 0 )
    {
      if( SD_ReadBlock(sector * 512, (uint32_t *)buff, 512) != SD_OK )
        return RES_ERROR;    
    }
    else
    {
	    if( NAND_Read(sector * 512, (uint32_t *)buff, 512) != NAND_OK )
        return RES_ERROR;
    }
    sector ++;
    count --;
    buff += 512;
  }
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
  while( count )
  {
    if( drv == 0 )
    {
      if( SD_WriteBlock(sector * 512, (uint32_t *)buff, 512) != SD_OK )
        return RES_ERROR;    
    }
    else
    {
      if( NAND_Write(sector * 512, (uint32_t *)buff, 512) != NAND_OK )
        return RES_ERROR;
    }
    sector ++;
    count --;
    buff += 512;
  }
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get current time                                                      */
/*-----------------------------------------------------------------------*/

DWORD get_fattime ()
{
	return	((2006UL-1980) << 25)	      // Year = 2006
			| (2UL << 21)	      // Month = Feb
			| (9UL << 16)	      // Day = 9
			| (22U << 11)	      // Hour = 22
			| (30U << 5)	      // Min = 30
			| (0U >> 1)	      // Sec = 0
			;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
 		
DRESULT disk_ioctl (
	BYTE drv,		// Physical drive number (0) 
	BYTE ctrl,		// Control code
	void *buff		// Buffer to send/receive control data
)
{		
	DRESULT res= RES_OK;

  switch (ctrl) {
  case CTRL_SYNC :		/// Make sure that no pending write process
    if( drv == 0 )
      res = SD_GetTransferState() == SD_NO_TRANSFER ? RES_OK : RES_ERROR;
    else
      res = FSMC_NAND_GetStatus() == NAND_READY ? RES_OK : RES_ERROR;
    break;

  case GET_SECTOR_COUNT :	  // Get number of sectors on the disk (DWORD)
    if( drv == 0 )
      *( DWORD* )buff = 131072; // [TODO] we should actually remember the disk size!
    else
      *( DWORD* )buff = 65536;
    res = RES_OK;
    break;

  case GET_SECTOR_SIZE :	  // Get R/W sector size (WORD) 
    *(WORD*)buff = 512;
    res = RES_OK;
    break;

  case GET_BLOCK_SIZE :	    // Get erase block size in unit of sector (DWORD)
    *(DWORD*)buff = 32;
    res = RES_OK;
    }
	  
	return res;
}

void disk_timerproc( void )
{
}

#endif // #if defined( BUILD_MMCFS ) && defined( MMCFS_SDIO_STM32 )

