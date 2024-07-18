/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.c
  * @author  MCD Application Team
  * @brief   Application of the SubGHz_Phy Middleware
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "subghz_phy_version.h"
#include "subg_command.h"
#include "subg_at.h"
#include "cmsis_os.h"
#include "test_rf.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
osThreadId_t Thd_VcomProcessId;

const osThreadAttr_t Thd_VcomProcess_attr =
{
  .name = CFG_VCOM_PROCESS_NAME,
  .attr_bits = CFG_VCOM_PROCESS_ATTR_BITS,
  .cb_mem = CFG_VCOM_PROCESS_CB_MEM,
  .cb_size = CFG_VCOM_PROCESS_CB_SIZE,
  .stack_mem = CFG_VCOM_PROCESS_STACK_MEM,
  .priority = CFG_VCOM_PROCESS_PRIORITY,
  .stack_size = CFG_VCOM_PROCESS_STACK_SIZE
};
static void Thd_VcomProcess(void *argument);

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  call back when LoRaWan Stack needs update
  */
static void CmdProcessNotify(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  CMD_Init(CmdProcessNotify);

  /* USER CODE BEGIN SubghzApp_Init_1 */

  /* USER CODE END SubghzApp_Init_1 */

  Thd_VcomProcessId = osThreadNew(Thd_VcomProcess, NULL, &Thd_VcomProcess_attr);
  if (Thd_VcomProcessId == NULL)
  {
    Error_Handler();
  }
  /* Create the semaphore for RfTest.  */
  TST_Semaphore_Init();

  /* USER CODE BEGIN SubghzApp_Init_2 */

  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void CmdProcessNotify(void)
{
  /* USER CODE BEGIN CmdProcessNotify_1 */

  /* USER CODE END CmdProcessNotify_1 */
  osThreadFlagsSet(Thd_VcomProcessId, 1);
  /* USER CODE BEGIN CmdProcessNotify_2 */

  /* USER CODE END CmdProcessNotify_2 */
}

static void Thd_VcomProcess(void *argument)
{
  /* USER CODE BEGIN Thd_VcomProcess_1 */

  /* USER CODE END Thd_VcomProcess_1 */
  UNUSED(argument);
  for (;;)
  {
    osThreadFlagsWait(1, osFlagsWaitAny, osWaitForever);
    CMD_Process(); /*what you want to do*/
  }
  /* USER CODE BEGIN Thd_VcomProcess_2 */

  /* USER CODE END Thd_VcomProcess_2 */
}
/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
