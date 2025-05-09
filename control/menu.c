/**
  ******************************************************************************
  * @file    IAP_Main/Src/menu.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
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
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
char aFileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
int SerialDownload(void);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
int SerialDownload(void)
{
  char number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = Ymodem_Receive( &size );
  if (result == COM_OK)
  {
    Serial_PutString("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString("\n\r Size: ");
    Serial_PutString(number);
    Serial_PutString(" Bytes\r\n");
    Serial_PutString("-------------------\n");
    return 0;
  }
  else if (result == COM_LIMIT)
  {
    Serial_PutString("\n\n\rThe image size is higher than the allowed space memory!\n\r");
  }
  else if (result == COM_DATA)
  {
    Serial_PutString("\n\n\rVerification failed!\n\r");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString("\r\n\nAborted by user.\n\r");
  }
  else
  {
    Serial_PutString("\n\rFailed to receive the file!\n\r");
  }
  return 1;
}

/**
  * @brief  Upload a file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(void)
{
  uint8_t status = 0;

  Serial_PutString("\n\n\rSelect Receive File\n\r");

  HAL_UART_Receive(&UartHandle, &status, 1, RX_TIMEOUT);
  if ( status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)APPLICATION_ADDRESS, (const uint8_t*)"UploadedFlashImage.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
      Serial_PutString("\n\rError Occurred while Transmitting File\n\r");
    }
    else
    {
      Serial_PutString("\n\rFile uploaded successfully \n\r");
    }
  }
}

#if 0
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
/* Avoids the semihosting issue */
__asm("  .global __ARM_use_no_argv\n");
#elif defined(__GNUC__)
/* Disables part of C/C++ runtime startup/teardown */
void __libc_init_array (void) {}
#endif

#if defined(__CC_ARM)
__asm void modify_stack_pointer_and_start_app(uint32_t r0_sp, uint32_t r1_pc)
{
    MOV SP, R0
    BX R1
}
#elif defined(__GNUC__)
void modify_stack_pointer_and_start_app(uint32_t r0_sp, uint32_t r1_pc)
{
    uint32_t z = 0;
    __asm volatile (  "msr    control, %[z]   \n\t"
                      "isb                    \n\t"
                      "mov    sp, %[r0_sp]    \n\t"
                      "bx     %[r1_pc]"
                      :
                      :   [z] "l" (z),
                      [r0_sp] "l" (r0_sp),
                      [r1_pc] "l" (r1_pc)
                   );
}
#else
#error "Unknown compiler!"
#endif
#else
void modify_stack_pointer_and_start_app(uint32_t r0_sp, uint32_t r1_pc)
{
    Serial_PutString("Start program execution......\r\n\n");
    /* execute the new program */
    JumpAddress = *(__IO uint32_t*) r1_pc;
    /* Jump to user application */
    JumpToApplication = (pFunction) JumpAddress;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*) r0_sp);
    JumpToApplication();
}
#endif
/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
  uint32_t tmp = *(__IO uint32_t*) (FLASH_UPGRADE_FLAG_ADDR);
    int skip_key = 0;
  uint8_t key = 0;
    _dbg_printf("enter bootloader, flash_upgrade_flag_value: %X\n", tmp);
  if(tmp != FLASH_UPGRADE_FLAG_VALUE) {
      modify_stack_pointer_and_start_app(APPLICATION_ADDRESS, APPLICATION_ADDRESS+4);
      return;
  } else {
      skip_key = 1;
      key = '1';
  }

  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  FlashProtection = FLASH_If_GetWriteProtectionStatus();

  while (1)
  {

    Serial_PutString("\r\n=================== Main Menu ============================\r\n\n");
    Serial_PutString("  Download image to the internal Flash ----------------- 1\r\n\n");
    Serial_PutString("  Upload image from the internal Flash ----------------- 2\r\n\n");
    Serial_PutString("  Execute the loaded application ----------------------- 3\r\n\n");


    if(FlashProtection != FLASHIF_PROTECTION_NONE)
    {
      Serial_PutString("  Disable the write protection ------------------------- 4\r\n\n");
    }
    else
    {
      Serial_PutString("  Enable the write protection -------------------------- 4\r\n\n");
    }
    Serial_PutString("==========================================================\r\n\n");

    /* Clean the input path */
    __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

    if(!skip_key) {
        /* Receive key */
        HAL_UART_Receive(&UartHandle, &key, 1, RX_TIMEOUT);
    } else {
        skip_key = 0;
    }

    switch (key)
    {
    case '1' :
      /* Download user application in the Flash */
        {
          #ifdef YS_BOARD
            int ret = SerialDownload();
            if(!ret) {
                uint32_t tmp[FLASH_NB_32BITWORD_IN_FLASHWORD]={0};
                HAL_FLASH_Unlock();
                uint32_t ok = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, FLASH_UPGRADE_FLAG_ADDR, (uint32_t)(tmp));
                HAL_FLASH_Lock();
                if(ok != HAL_OK) {
                    _dbg_printf("write to upgrade flag failed: %d\n", ok);
                } else {
                    HAL_NVIC_SystemReset(); //reboot
                }
            }
          #endif
            break;
        }
    case '2' :
      /* Upload user application from the Flash */
      SerialUpload();
      break;
    case '3' :
        modify_stack_pointer_and_start_app(APPLICATION_ADDRESS, APPLICATION_ADDRESS+4);
      break;
    case '4' :
      if (FlashProtection != FLASHIF_PROTECTION_NONE)
      {
        /* Disable the write protection */
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_DISABLE) == FLASHIF_OK)
        {
          Serial_PutString("Write Protection disabled...will reboot\r\n");
            HAL_NVIC_SystemReset(); //reboot
        }
        else
        {
          Serial_PutString("Error: Flash write un-protection failed...\r\n");
        }
      }
      else
      {
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_ENABLE) == FLASHIF_OK)
        {
          Serial_PutString("Write Protection enabled...will reboot\r\n");
            HAL_NVIC_SystemReset(); //reboot
        }
        else
        {
          Serial_PutString("Error: Flash write protection failed...\r\n");
        }
      }
      break;
	default:
	Serial_PutString("Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
	break;
    }
  }
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
