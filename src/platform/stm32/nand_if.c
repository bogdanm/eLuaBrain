/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : nand_if.c
* Author             : MCD Application Team
* Version            : V1.1.2
* Date               : 09/22/2008
* Description        : manage NAND operationx state machine
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "nand_if.h"
//#include "mass_mal.h"
#include "fsmc_nand.h"
//#include "memory.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* extern variables-----------------------------------------------------------*/
//extern u32 SCSI_LBA;
//extern u32 SCSI_BlkLen;
/* Private variables ---------------------------------------------------------*/
u16 LUT[1024]; /* Look Up Table Buffer */
WRITE_STATE Write_State;
BLOCK_STATE Block_State;
NAND_ADDRESS wAddress, fAddress;
u16  phBlock, LogAddress, Initial_Page, CurrentZone = 0;
u16  Written_Pages = 0;

#define SCSI_BlkLen  1

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static u16 NAND_CleanLUT(u8 ZoneNbr);
static NAND_ADDRESS NAND_GetAddress(u32 Address);
static u16 NAND_GetFreeBlock(void);
static u16 NAND_Write_Cleanup(void);
SPARE_AREA ReadSpareArea(u32 address);
static u16 NAND_Copy(NAND_ADDRESS Address_Src, NAND_ADDRESS Address_Dest, u16 PageToCopy);
static NAND_ADDRESS NAND_ConvertPhyAddress(u32 Address);
static u16 NAND_BuildLUT(u8 ZoneNbr);

/*******************************************************************************
* Function Name  : NAND_Init
* Description    : Init NAND Interface
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
u16 NAND_Init(void)
{
  u16 Status = NAND_OK;

  FSMC_NAND_Init();
  Status = NAND_BuildLUT(0);
  Write_State = WRITE_IDLE;
  return Status;
}

/*******************************************************************************
* Function Name  : NAND_Write
* Description    : write one sector by once
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
u16 NAND_Write(u32 Memory_Offset, u32 *Writebuff, u16 Transfer_Length)
{
  /* check block status and calculate start and end addreses */
  wAddress    = NAND_GetAddress(Memory_Offset / 512);

  /*check Zone: if second zone is requested build second LUT*/
  if (wAddress.Zone != CurrentZone)
  {
    CurrentZone = wAddress.Zone;
    NAND_BuildLUT(CurrentZone);
  }

  phBlock     = LUT[wAddress.Block]; /* Block Index + flags */
  LogAddress  = wAddress.Block ; /* save logical block */

  /*  IDLE state  */
  /****************/
  if ( Write_State == WRITE_IDLE)
  {/* Idle state */

    if (phBlock & USED_BLOCK)
    { /* USED BLOCK */

      Block_State = OLD_BLOCK;
      /* Get a free Block for swap */
      fAddress.Block = NAND_GetFreeBlock();
      fAddress.Zone  = wAddress.Zone;
      Initial_Page = fAddress.Page  = wAddress.Page;

      /* write the new page */
      FSMC_NAND_WriteSmallPage((u8 *)Writebuff, fAddress, PAGE_TO_WRITE);
      Written_Pages++;

      /* get physical block */
      wAddress.Block = phBlock & 0x3FF;


      if (Written_Pages == SCSI_BlkLen)
      {
        NAND_Write_Cleanup();
        Written_Pages = 0;
        return NAND_OK;
      }
      else
      {
        if (wAddress.Page == (NAND_BLOCK_SIZE - 1))
        {
          NAND_Write_Cleanup();
          return NAND_OK;
        }
        Write_State = WRITE_ONGOING;
        return NAND_OK;
      }
    }
    else
    {/* UNUSED BLOCK */

      Block_State = UNUSED_BLOCK;
      /* write the new page */
      wAddress.Block = phBlock & 0x3FF;
      FSMC_NAND_WriteSmallPage( (u8 *)Writebuff , wAddress, PAGE_TO_WRITE);

      Written_Pages++;
      if (Written_Pages == SCSI_BlkLen)
      {
        Written_Pages = 0;
        NAND_Write_Cleanup();
        return NAND_OK;
      }
      else
      {
        Write_State = WRITE_ONGOING;
        return NAND_OK;
      }
    }
  }
  /* WRITE state */
  /***************/
  if ( Write_State == WRITE_ONGOING)
  {/* Idle state */
    if (phBlock & USED_BLOCK)
    { /* USED BLOCK */

      wAddress.Block = phBlock & 0x3FF;
      Block_State = OLD_BLOCK;
      fAddress.Page  = wAddress.Page;

      /* check if next pages are in next block */
      if (wAddress.Page == (NAND_BLOCK_SIZE - 1))
      {
        /* write Last page  */
        FSMC_NAND_WriteSmallPage( (u8 *)Writebuff , fAddress, PAGE_TO_WRITE);
        Written_Pages++;
        if (Written_Pages == SCSI_BlkLen)
        {
          Written_Pages = 0;
        }
        /* Clean up and Update the LUT */
        NAND_Write_Cleanup();
        Write_State = WRITE_IDLE;
        return NAND_OK;
      }

      /* write next page */
      FSMC_NAND_WriteSmallPage( (u8 *)Writebuff , fAddress, PAGE_TO_WRITE);
      Written_Pages++;
      if (Written_Pages == SCSI_BlkLen)
      {
        Write_State = WRITE_IDLE;
        NAND_Write_Cleanup();
        Written_Pages = 0;
      }

    }
    else
    {/* UNUSED BLOCK */
      wAddress.Block = phBlock & 0x3FF;
      /* check if it is the last page in prev block */
      if (wAddress.Page == (NAND_BLOCK_SIZE - 1))
      {
        /* write Last page  */
        FSMC_NAND_WriteSmallPage( (u8 *)Writebuff , wAddress, PAGE_TO_WRITE);
        Written_Pages++;
        if (Written_Pages == SCSI_BlkLen)
        {
          Written_Pages = 0;
        }

        /* Clean up and Update the LUT */
        NAND_Write_Cleanup();
        Write_State = WRITE_IDLE;



        return NAND_OK;
      }
      /* write next page in same block */
      FSMC_NAND_WriteSmallPage( (u8 *)Writebuff , wAddress, PAGE_TO_WRITE);
      Written_Pages++;
      if (Written_Pages == SCSI_BlkLen)
      {
        Write_State = WRITE_IDLE;
        NAND_Write_Cleanup();
        Written_Pages = 0;
      }
    }
  }
  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
u16 NAND_Read(u32 Memory_Offset, u32 *Readbuff, u16 Transfer_Length)
{
  NAND_ADDRESS phAddress;

  phAddress = NAND_GetAddress(Memory_Offset / 512);

  if (phAddress.Zone != CurrentZone)
  {
    CurrentZone = phAddress.Zone;
    NAND_BuildLUT(CurrentZone);
  }

  if (LUT [phAddress.Block] & BAD_BLOCK)
  {
    return NAND_FAIL;
  }
  else
  {
    phAddress.Block = LUT [phAddress.Block] & ~ (USED_BLOCK | VALID_BLOCK);
    FSMC_NAND_ReadSmallPage ( (u8 *)Readbuff , phAddress, Transfer_Length / 512);
  }
  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_CleanLUT
* Description    : Erase old blocks & rebuild the look up table
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_CleanLUT (u8 ZoneNbr)
{
#ifdef WEAR_LEVELLING_SUPPORT
  u16 BlockIdx, LUT_Item;
#endif
  /* Rebuild the LUT for the current zone */
  NAND_BuildLUT (ZoneNbr);

#ifdef WEAR_LEVELLING_SUPPORT
  /* Wear Leveling : circular use of free blocks */
  LUT_Item = LUT [BlockIdx]
             for (BlockIdx == MAX_LOG_BLOCKS_PER_ZONE ; BlockIdx < MAX_LOG_BLOCKS_PER_ZONE + WEAR_DEPTH ; BlockIdx++)
             {
               LUT [BlockIdx] = LUT [BlockIdx + 1];
             }
             LUT [ MAX_LOG_BLOCKS_PER_ZONE + WEAR_DEPTH - 1] = LUT_Item ;
#endif

  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_GetAddress
* Description    : Translate logical address into a phy one
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static NAND_ADDRESS NAND_GetAddress (u32 Address)
{
  NAND_ADDRESS Address_t;

  Address_t.Page  = Address & (NAND_BLOCK_SIZE - 1);
  Address_t.Block = Address / NAND_BLOCK_SIZE;
  Address_t.Zone = 0;

  while (Address_t.Block >= MAX_LOG_BLOCKS_PER_ZONE)
  {
    Address_t.Block -= MAX_LOG_BLOCKS_PER_ZONE;
    Address_t.Zone++;
  }
  return Address_t;
}

/*******************************************************************************
* Function Name  : NAND_GetFreeBlock
* Description    : Look for a free block for data exchange
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_GetFreeBlock (void)
{
  return LUT[MAX_LOG_BLOCKS_PER_ZONE]& ~(USED_BLOCK | VALID_BLOCK);
}

/*******************************************************************************
* Function Name  : ReadSpareArea
* Description    : Check used block
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
SPARE_AREA ReadSpareArea (u32 address)
{
  SPARE_AREA t;
  u8 Buffer[16];
  NAND_ADDRESS address_s;
  address_s = NAND_ConvertPhyAddress(address);
  FSMC_NAND_ReadSpareArea(Buffer , address_s, 1) ;

  t = *(SPARE_AREA *)Buffer;

  return t;
}

/*******************************************************************************
* Function Name  : NAND_Copy
* Description    : Copy page
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_Copy (NAND_ADDRESS Address_Src, NAND_ADDRESS Address_Dest, u16 PageToCopy)
{
  u8 Copybuff[512];
  for ( ; PageToCopy > 0 ; PageToCopy-- )
  {
    FSMC_NAND_ReadSmallPage  ((u8 *)Copybuff, Address_Src , 1 );
    FSMC_NAND_WriteSmallPage ((u8 *)Copybuff, Address_Dest, 1);
    FSMC_NAND_AddressIncrement(&Address_Src);
    FSMC_NAND_AddressIncrement(&Address_Dest);
  }

  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_Format
* Description    : Format the entire NAND flash
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
u16 NAND_Format (void)
{
  NAND_ADDRESS phAddress;
  SPARE_AREA  SpareArea;
  u32 BlockIndex;

  for (BlockIndex = 0 ; BlockIndex < NAND_ZONE_SIZE * NAND_MAX_ZONE; BlockIndex++)
  {
    phAddress = NAND_ConvertPhyAddress(BlockIndex * NAND_BLOCK_SIZE );
    SpareArea = ReadSpareArea(BlockIndex * NAND_BLOCK_SIZE);
   
    if((SpareArea.DataStatus != 0)||(SpareArea.BlockStatus != 0)){
        FSMC_NAND_EraseBlock (phAddress);
    }  
  }
  NAND_BuildLUT(0);
  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_Write_Cleanup
* Description    : None
* Input          : None
* Output         : None
* Return         : Status
*******************************************************************************/
static u16 NAND_Write_Cleanup (void)
{
  u16  tempSpareArea [8];
  u16  Page_Back;

  if ( Block_State == OLD_BLOCK )
  {
    /* precopy old first pages */
    if (Initial_Page != 0)
    {
      Page_Back = wAddress.Page;
      fAddress.Page = wAddress.Page = 0;
      NAND_Copy (wAddress, fAddress, Initial_Page);
      wAddress.Page =  Page_Back ;
    }

    /* postcopy remaining pages */
    if ((NAND_BLOCK_SIZE - (wAddress.Page + 1)) != 0)
    {
      FSMC_NAND_AddressIncrement(&wAddress);
      fAddress.Page = wAddress.Page;
      NAND_Copy (wAddress, fAddress, NAND_BLOCK_SIZE - wAddress.Page);
    }

    /* assign logical address to new block */
    tempSpareArea [0] = LogAddress | USED_BLOCK ;
    tempSpareArea [1] = 0xFFFF;
    tempSpareArea [2] = 0xFFFF;

    fAddress.Page     = 0x00;
    FSMC_NAND_WriteSpareArea( (u8 *)tempSpareArea , fAddress , 1);

    /* erase old block */
    FSMC_NAND_EraseBlock(wAddress);
    NAND_CleanLUT(wAddress.Zone);
  }
  else
  {/* unused block case */
    /* assign logical address to the new used block */
    tempSpareArea [0] = LogAddress | USED_BLOCK ;
    tempSpareArea [1] = 0xFFFF;
    tempSpareArea [2] = 0xFFFF;

    wAddress.Page     = 0x00;
    FSMC_NAND_WriteSpareArea((u8 *)tempSpareArea , wAddress, 1);
    NAND_CleanLUT(wAddress.Zone);
  }
  return NAND_OK;
}

/*******************************************************************************
* Function Name  : NAND_ConvertPhyAddress
* Description    : None
* Input          : physical Address
* Output         : None
* Return         : Status
*******************************************************************************/
static NAND_ADDRESS NAND_ConvertPhyAddress (u32 Address)
{
  NAND_ADDRESS Address_t;

  Address_t.Page  = Address & (NAND_BLOCK_SIZE - 1);
  Address_t.Block = Address / NAND_BLOCK_SIZE;
  Address_t.Zone = 0;

  while (Address_t.Block >= MAX_PHY_BLOCKS_PER_ZONE)
  {
    Address_t.Block -= MAX_PHY_BLOCKS_PER_ZONE;
    Address_t.Zone++;
  }
  return Address_t;
}

/*******************************************************************************
* Function Name  : NAND_BuildLUT
* Description    : Build the look up table
* Input          : None
* Output         : None
* Return         : Status
* !!!! NOTE : THIS ALGORITHM IS A SUBJECT OF PATENT FOR STMICROELECTRONICS !!!!!
*******************************************************************************/
static u16 NAND_BuildLUT (u8 ZoneNbr)
{

  u16  pBadBlock, pCurrentBlock, pFreeBlock;
  SPARE_AREA  SpareArea;
  /*****************************************************************************
                                  1st step : Init.
  *****************************************************************************/
  /*Init the LUT (assume all blocks free) */
  for (pCurrentBlock = 0 ; pCurrentBlock < MAX_PHY_BLOCKS_PER_ZONE ; pCurrentBlock++)
  {
    LUT[pCurrentBlock] = FREE_BLOCK;  /* 12th bit is set to 1 */
  }

  /* Init Pointers */
  pBadBlock    = MAX_PHY_BLOCKS_PER_ZONE - 1;
  pCurrentBlock = 0;

  /*****************************************************************************
                         2nd step : locate used and bad blocks
  *****************************************************************************/

  while (pCurrentBlock < MAX_PHY_BLOCKS_PER_ZONE)
  {

    SpareArea = ReadSpareArea(pCurrentBlock * NAND_BLOCK_SIZE + (ZoneNbr * NAND_BLOCK_SIZE * MAX_PHY_BLOCKS_PER_ZONE));

    if ((SpareArea.DataStatus == 0) || (SpareArea.BlockStatus == 0))
    {

      LUT[pBadBlock--]    |= pCurrentBlock | (u16)BAD_BLOCK ;
      LUT[pCurrentBlock] &= (u16)~FREE_BLOCK;
      if (pBadBlock == MAX_LOG_BLOCKS_PER_ZONE)
      {
        return NAND_FAIL;
      }
    }
    else if (SpareArea.LogicalIndex != 0xFFFF)
    {

      LUT[SpareArea.LogicalIndex & 0x3FF] |= pCurrentBlock | VALID_BLOCK | USED_BLOCK;
      LUT[pCurrentBlock] &= (u16)( ~FREE_BLOCK);
    }
    pCurrentBlock++ ;
  }

  /*****************************************************************************
     3rd step : locate Free Blocks by scanning the LUT already built partially
  *****************************************************************************/
  pFreeBlock = 0;
  for (pCurrentBlock = 0 ; pCurrentBlock < MAX_PHY_BLOCKS_PER_ZONE ; pCurrentBlock++ )
  {

    if ( !(LUT[pCurrentBlock]& USED_BLOCK))
    {
      do
      {
        if (LUT[pFreeBlock] & FREE_BLOCK)
        {

          LUT [pCurrentBlock] |= pFreeBlock;
          LUT [pFreeBlock]   &= ~FREE_BLOCK;
          break;
        }
        pFreeBlock++;
      }
      while ( pFreeBlock < MAX_PHY_BLOCKS_PER_ZONE );
    }
  }
  return NAND_OK;
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
