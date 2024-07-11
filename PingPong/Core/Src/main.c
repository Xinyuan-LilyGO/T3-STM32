/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "rtc.h"
#include "app_subghz_phy.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include "rtc.h"
#include "usart.h"
#include "subghz.h"
#include "lib_log.h"

#include "utilities_conf.h"
#include "stm32_seq.h"
#include "stm32_adv_trace.h"

#include "usart_if.h"
#include "sys_app.h"
#include "app_version.h"
#include "subghz_phy_version.h"

#include "subghz_phy_app.h"
#include "radio.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "subg_command.h"
#include "subg_at.h"
/* USER CODE END Includes */

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
RTC_TimeTypeDef RTC_Time;
RTC_DateTypeDef RTC_Date;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void MyTask0(void)
{
  LOG_INFO("My Task is taggert 000\n");
  LED2_TRI;
}

void MyTask1(void)
{
  LOG_USER("My Task is taggert 111\n");
  LED2_TRI;
}

uint16_t task_idx = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == BOOT_Pin) {
    LOG_INFO("BOOT PRESS\n");
    if(task_idx % 2 == 0) {
      // UTIL_SEQ_SetTask((1 << CFG_SEQ_Prio_0), UTIL_SEQ_RFU);
    }else{
      UTIL_SEQ_SetTask((1 << CFG_TASK_ID_BOOT_BTN), UTIL_SEQ_RFU);
    }

    task_idx++;
    // LED2_TRI;
  }
}

#define BUF_SIZE 16
uint8_t usart_buf[BUF_SIZE]={0};
uint16_t usart_widx = 0;
uint16_t usart_cnt = 0;
uint16_t buff_over = 0;

void USART_GetChar(uint8_t *rxChar, uint16_t size, uint8_t error)
{
  if(*rxChar != '\n') {
    
    if(usart_cnt < BUF_SIZE - 1) {
      usart_buf[usart_cnt] = *rxChar;
      usart_cnt++;
    }
  } else {
    usart_buf[usart_cnt] = '\0';
    buff_over = 1;
  }

  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_Vcom), UTIL_SEQ_RFU);
}

void USART_process(void)
{
  LOG_TRACE("USART_process:[%s]\n", usart_buf);
  if(buff_over == 1) {
    buff_over = 0;
    usart_cnt = 0;
  }
}

static void CmdProcessNotify(void)
{
  /* USER CODE BEGIN CmdProcessNotify_1 */

  /* USER CODE END CmdProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_Vcom), UTIL_SEQ_RFU);
  /* USER CODE BEGIN CmdProcessNotify_2 */

  /* USER CODE END CmdProcessNotify_2 */
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  UTIL_SEQ_Init();

  UTIL_ADV_TRACE_Init();
  // UTIL_ADV_TRACE_RegisterTimeStampFunction(TimestampNow);
  UTIL_ADV_TRACE_SetVerboseLevel(VLEVEL_M);
  // UTIL_ADV_TRACE_StartRxProcess(USART_GetChar);

  segger_rtt_init("Hello SEGGER RTT\n");
  uint32_t LED_tick = HAL_GetTick();
  uint32_t RTC_tick = HAL_GetTick();

  
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  LORA_TX;
  MX_SubGHz_Phy_Init();
  /* USER CODE BEGIN 2 */
#if 0
  /*Initialize the terminal */
  // UTIL_ADV_TRACE_Init();
  // UTIL_ADV_TRACE_RegisterTimeStampFunction(TimestampNow);
  // UTIL_ADV_TRACE_SetVerboseLevel(VLEVEL_M);
  // UTIL_ADV_TRACE_StartRxProcess(USART_GetChar);

  // CMD_Init(CmdProcessNotify);
  // UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Vcom), UTIL_SEQ_RFU, CMD_Process);

  
  // UTIL_ADV_TRACE_StartRxProcess(CMD_GetChar);
  // HAL_SUBGHZ_Init(&hsubghz);
  // UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Vcom), UTIL_SEQ_RFU, USART_process);
  // UTIL_SEQ_RegTask((1 << CFG_TASK_ID_BOOT_BTN), UTIL_SEQ_RFU, MyTask1);

  RTC_Time.Hours = 14;
  RTC_Time.Minutes = 20;
  RTC_Time.Seconds = 0;
  RTC_Date.Year = 24;
  RTC_Date.Month = RTC_MONTH_JULY;
  RTC_Date.Date = 8;
  RTC_Date.WeekDay = RTC_WEEKDAY_MONDAY;
  
  HAL_RTC_SetTime(&hrtc, &RTC_Time, RTC_FORMAT_BCD);
  HAL_RTC_SetDate(&hrtc, &RTC_Date, RTC_FORMAT_BCD);
  
  segger_rtt_init("Hello SEGGER RTT\n");
  char txBuffer[] = "Hello World\n";

  while(1)
  {
    if(HAL_GetTick() - LED_tick > 200) {
      LED_tick = HAL_GetTick();
      // HAL_SUBGHZ_WriteBuffer(&hsubghz, 0x00, (uint8_t *)txBuffer, strlen(txBuffer));
      LED1_TRI;
    }

    // if(HAL_GetTick() - RTC_tick > 1000) {
      // RTC_tick = HAL_GetTick();
      // AT_PPRINTF("Hello\n");

      // vcom_Trace_DMA((uint8_t *)txBuffer, strlen(txBuffer));
      // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)txBuffer, strlen(txBuffer));

      /* Get SubGHY_Phy APP version*/
      // APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
      //         (uint8_t)(APP_VERSION_MAIN),
      //         (uint8_t)(APP_VERSION_SUB1),
      //         (uint8_t)(APP_VERSION_SUB2));

      // /* Get MW SubGhz_Phy info */
      // APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
      //         (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
      //         (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
      //         (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

      
      // HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BCD);
      // HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BCD);
      // LOG_INFO("[%02d]%02d/%02d/%02d %02d:%02d:%02d\r\n",RTC_Date.WeekDay, 2000 + RTC_Date.Year, RTC_Date.Month, RTC_Date.Date,
      //               RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
    // }
    
    // UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
    MX_SubGHz_Phy_Process();
  }
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 
  while (1)
  {
    if(HAL_GetTick() - LED_tick > 200) {
      LED_tick = HAL_GetTick();
      LED2_TRI;
    }

    // if(HAL_GetTick() - RTC_tick > 1000) {
    //   RTC_tick = HAL_GetTick();
    //   HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BCD);
    //   HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BCD);
    //   /* Display date Format : yy/mm/dd */
    //   LOG_INFO("[%02d]%02d/%02d/%02d %02d:%02d:%02d\r\n",RTC_Date.WeekDay, 2000 + RTC_Date.Year, RTC_Date.Month, RTC_Date.Date,
    //                 RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
    //   LED1_TRI;
    // }
    /* USER CODE END WHILE */
    MX_SubGHz_Phy_Process();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSEDiv = RCC_HSE_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 6;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
