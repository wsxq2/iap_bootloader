/**
  ******************************************************************************
  * @file    IAP_Main/Src/flash_if.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the memory related operation functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/** @addtogroup STM32F1xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_If_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
#ifdef YS_BOARD
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR);
#else
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
#endif
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  start: start of user flash area
  * @retval FLASHIF_OK : user flash area successfully erased
  *         FLASHIF_ERASEKO : error occurred
  */
uint32_t FLASH_If_Erase(uint32_t start)
{
  uint32_t NbrOfPages = 0;
  uint32_t PageError = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  HAL_StatusTypeDef status = HAL_OK;

  /* Unlock the Flash to enable the flash control register access *************/ 
  HAL_FLASH_Unlock();

#ifdef YS_BOARD
  NbrOfPages = (USER_FLASH_END_ADDRESS - start)/FLASH_SECTOR_SIZE;
  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.Sector = (start - FLASH_BASE)/FLASH_SECTOR_SIZE;
  pEraseInit.Banks = FLASH_BANK_1;
  pEraseInit.NbSectors = NbrOfPages;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#else
  /* Get the sector where start the user flash area */
  NbrOfPages = (USER_FLASH_END_ADDRESS - start)/FLASH_PAGE_SIZE;

  pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
  pEraseInit.PageAddress = start;
  pEraseInit.Banks = FLASH_BANK_1;
  pEraseInit.NbPages = NbrOfPages;
#endif
  status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  if (status != HAL_OK)
  {
    /* Error occurred while page erase */
    return FLASHIF_ERASEKO;
  }

  return FLASHIF_OK;
}

/* Public functions ---------------------------------------------------------*/
/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of data buffer (unit is 32-bit word)
  * @retval uint32_t 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  uint32_t i = 0;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

#ifdef YS_BOARD
#include <string.h>
  const size_t bytes = sizeof(uint32_t)*FLASH_NB_32BITWORD_IN_FLASHWORD;
  uint32_t tmp[FLASH_NB_32BITWORD_IN_FLASHWORD]={0};
  if(length % FLASH_NB_32BITWORD_IN_FLASHWORD != 0) {
      if(length / FLASH_NB_32BITWORD_IN_FLASHWORD == 0) {
          memcpy(tmp, p_source, length);
          length = FLASH_NB_32BITWORD_IN_FLASHWORD;
          p_source = tmp;
      } else {
          return FLASHIF_WRITING_NOT_ALIGN8_ERRROR;
      }
  }
  for (i = 0; (i < length/FLASH_NB_32BITWORD_IN_FLASHWORD) && (destination <= (USER_FLASH_END_ADDRESS-bytes)); i++)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, destination, (uint32_t)(p_source)) == HAL_OK)
    {
     /* Check the written value */
        if (memcmp((void*)destination, (void*)p_source, bytes)!=0)
      {
        /* Flash content doesn't match SRAM content */
        return(FLASHIF_WRITINGCTRL_ERROR);
      }
      /* Increment FLASH destination address */
      destination += bytes;
      p_source += FLASH_NB_32BITWORD_IN_FLASHWORD;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      return (FLASHIF_WRITING_ERROR);
    }
  }
#else
  for (i = 0; (i < length) && (destination <= (USER_FLASH_END_ADDRESS-4)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destination, *(uint32_t*)(p_source+i)) == HAL_OK)
    {
     /* Check the written value */
      if (*(uint32_t*)destination != *(uint32_t*)(p_source+i))
      {
        /* Flash content doesn't match SRAM content */
        return(FLASHIF_WRITINGCTRL_ERROR);
      }
      /* Increment FLASH destination address */
      destination += 4;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      return (FLASHIF_WRITING_ERROR);
    }
  }
#endif

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return (FLASHIF_OK);
}

/**
  * @brief  Returns the write protection status of application flash area.
  * @param  None
  * @retval If a sector in application area is write-protected returned value is a combinaison
            of the possible values : FLASHIF_PROTECTION_WRPENABLED, FLASHIF_PROTECTION_PCROPENABLED, ...
  *         If no sector is write-protected FLASHIF_PROTECTION_NONE is returned.
  */
uint32_t FLASH_If_GetWriteProtectionStatus(void)
{
  uint32_t ProtectedPAGE = FLASHIF_PROTECTION_NONE;
    FLASH_OBProgramInitTypeDef OptionsBytesStruct={0};

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

#ifdef YS_BOARD
  OptionsBytesStruct.Banks = FLASH_BANK_1;
#endif

  /* Check if there are write protected sectors inside the user flash area ****/
  HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  /* Get pages already write protected ****************************************/
#ifdef YS_BOARD
  ProtectedPAGE = (OptionsBytesStruct.WRPSector) & FLASH_PAGE_TO_BE_PROTECTED;
#else
  ProtectedPAGE = ~(OptionsBytesStruct.WRPPage) & FLASH_PAGE_TO_BE_PROTECTED;
#endif
  //_dbg_printf("%s:%d:  WRPSector=%X, ProtectedPAGE=%X\n", __func__, __LINE__, OptionsBytesStruct.WRPSector, ProtectedPAGE);

  /* Check if desired pages are already write protected ***********************/
  if(ProtectedPAGE != 0)
  {
    /* Some sectors inside the user flash area are write protected */
    return FLASHIF_PROTECTION_WRPENABLED;
  }
  else
  { 
    /* No write protected sectors inside the user flash area */
    return FLASHIF_PROTECTION_NONE;
  }
}

/**
  * @brief  Configure the write protection status of user flash area.
  * @param  protectionstate : FLASHIF_WRP_DISABLE or FLASHIF_WRP_ENABLE the protection
  * @retval uint32_t FLASHIF_OK if change is applied.
  */
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate)
{
  uint32_t ProtectedPAGE = 0x0;
  FLASH_OBProgramInitTypeDef config_new, config_old;
  HAL_StatusTypeDef result = HAL_OK;
  
#ifdef YS_BOARD
  config_old.Banks = FLASH_BANK_1;
#endif

  /* Get pages write protection status ****************************************/
  HAL_FLASHEx_OBGetConfig(&config_old);

  /* The parameter says whether we turn the protection on or off */
  config_new.WRPState = (protectionstate == FLASHIF_WRP_ENABLE ? OB_WRPSTATE_ENABLE : OB_WRPSTATE_DISABLE);

  /* We want to modify only the Write protection */
  config_new.OptionType = OPTIONBYTE_WRP;
  
  /* No read protection, keep BOR and reset settings */
  config_new.RDPLevel = OB_RDP_LEVEL_0;
  config_new.USERConfig = config_old.USERConfig;  
  /* Get pages already write protected ****************************************/
#ifdef YS_BOARD
  ProtectedPAGE = FLASH_PAGE_TO_BE_PROTECTED;
#else
  ProtectedPAGE = config_old.WRPPage | FLASH_PAGE_TO_BE_PROTECTED;
#endif
  //_dbg_printf("%s:%d:  WRPSector=%X, ProtectedPAGE=%X\n", __func__, __LINE__, config_old.WRPSector, ProtectedPAGE);

  /* Unlock the Flash to enable the flash control register access *************/ 
  HAL_FLASH_Unlock();

  /* Unlock the Options Bytes *************************************************/
  HAL_FLASH_OB_Unlock();
  
  /* Erase all the option Bytes ***********************************************/
#ifdef YS_BOARD
    config_new.WRPSector    = ProtectedPAGE;
  config_new.Banks = FLASH_BANK_1;
    result = HAL_FLASHEx_OBProgram(&config_new);
#else
  result = HAL_FLASHEx_OBErase();

  if (result == HAL_OK)
  {
    config_new.WRPPage    = ProtectedPAGE;
    result = HAL_FLASHEx_OBProgram(&config_new);
  }
#endif

    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  
  return (result == HAL_OK ? FLASHIF_OK: FLASHIF_PROTECTION_ERRROR);
}
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
