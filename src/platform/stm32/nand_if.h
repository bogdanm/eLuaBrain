/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : nand_if.h
* Author             : MCD Application Team
* Version            : V1.1.2
* Date               : 09/22/2008
* Description        : All functions related to the NAND process
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NAND_IF_H
#define __NAND_IF_H

/* Includes ------------------------------------------------------------------*/
#include "type.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define NAND_OK   0
#define NAND_FAIL 1

#define FREE_BLOCK  (1 << 12 )
#define BAD_BLOCK   (1 << 13 )
#define VALID_BLOCK (1 << 14 )
#define USED_BLOCK  (1 << 15 )

#define MAX_PHY_BLOCKS_PER_ZONE  1024
#define MAX_LOG_BLOCKS_PER_ZONE  1000

/* Private Structures---------------------------------------------------------*/
typedef struct __SPARE_AREA {
	u16 LogicalIndex;
	u16 DataStatus;
	u16 BlockStatus;
} SPARE_AREA;	

typedef enum {
  WRITE_IDLE = 0,
  POST_WRITE,
  PRE_WRITE,
  WRITE_CLEANUP,
  WRITE_ONGOING  
}WRITE_STATE;  

typedef enum {
  OLD_BLOCK = 0,
  UNUSED_BLOCK
}BLOCK_STATE; 

/* Private macro -------------------------------------------------------------*/
//#define WEAR_LEVELLING_SUPPORT
#define WEAR_DEPTH         10
#define PAGE_TO_WRITE      (Transfer_Length/512)

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* exported functions --------------------------------------------------------*/
u16 NAND_Init (void);
u16 NAND_Write (u32 Memory_Offset, u32 *Writebuff, u16 Transfer_Length);
u16 NAND_Read  (u32 Memory_Offset, u32 *Readbuff, u16 Transfer_Length);
u16 NAND_Format (void);
SPARE_AREA ReadSpareArea (u32 address);

#endif

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
