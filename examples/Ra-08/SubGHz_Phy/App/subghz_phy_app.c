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
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "user.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  RX,
  RX_TIMEOUT,
  RX_ERROR,
  TX,
  TX_TIMEOUT,
} States_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */
static States_t State = RX;
/* Last  Received packer SNR (in Lora modulation)*/

extern uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
extern bool RxFinishFlag;
extern uint16_t RxBufferSize;/* Last  Received Buffer Size*/
extern int8_t RssiValue; /* Last  Received packer Rssi*/
extern int8_t SnrValue; /* Last  Received packer SNR (in Lora modulation)*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 */
static void OnTxDone(void);

/**
  * @brief Function to be executed on Radio Rx Done event
  * @param  payload ptr of buffer received
  * @param  size buffer size
  * @param  rssi
  * @param  LoraSnr_FskCfo
  */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

/**
  * @brief Function executed on Radio Tx Timeout event
  */
static void OnTxTimeout(void);

/**
  * @brief Function executed on Radio Rx Timeout event
  */
static void OnRxTimeout(void);

/**
  * @brief Function executed on Radio Rx Error event
  */
static void OnRxError(void);

/* USER CODE BEGIN PFP */
static void PrintPayloadHex(const uint8_t *payload, uint16_t size);
static void PrintPayloadString(const uint8_t *payload, uint16_t size);
static uint8_t Ra08Checksum8(const uint8_t *payload, uint16_t size);
static bool ParseRa08TransparentFrame(const uint8_t *frame, uint16_t size,
                                      uint16_t *from_addr, uint16_t *to_addr,
                                      const uint8_t **app_payload, uint16_t *app_size);

/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */

  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */

  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
  State = TX;
  user_on_radio_tx_done();
  /* USER CODE END OnTxDone */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */
  const uint8_t *display_payload = payload;
  uint16_t display_size = size;

#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
  SnrValue = LoraSnr_FskCfo;
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
#if ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  SnrValue = 0; /*not applicable in GFSK*/
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
  State = RX;

#if RA08_TRANSPARENT_COMPAT
  {
    uint16_t from_addr = 0U;
    uint16_t to_addr = 0U;
    const uint8_t *app_payload = NULL;
    uint16_t app_size = 0U;

    if (!ParseRa08TransparentFrame(payload, size, &from_addr, &to_addr, &app_payload, &app_size))
    {
      return;
    }

    if (to_addr != user_get_local_addr())
    {
      return;
    }

    display_payload = app_payload;
    display_size = app_size;
    user_set_last_rx_addr(from_addr, to_addr);
  }
#else
#endif

  APP_PRINTF("OnRxDone\r\n");
  APP_PRINTF("Recv:\r\n");
  if (user_is_rx_string_mode())
  {
    PrintPayloadString(display_payload, display_size);
  }
  else
  {
    PrintPayloadHex(display_payload, display_size);
  }
  APP_PRINTF("\r\nfrom: %u\r\n", (unsigned int)RxFromAddr);
  APP_PRINTF("rssi = %d, snr = %d\r\n", rssi, LoraSnr_FskCfo);

  LED1_TRI;
  
  memset(BufferRx, 0, MAX_APP_BUFFER_SIZE);
  RxBufferSize = display_size;
  RxFinishFlag = true;
  RssiValue = rssi;
  if (RxBufferSize <= MAX_APP_BUFFER_SIZE)
  {
    memcpy(BufferRx, display_payload, RxBufferSize);
  }
  /* USER CODE END OnRxDone */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
  State = TX_TIMEOUT;
  user_on_radio_tx_timeout();
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
  State = RX_TIMEOUT;
  user_on_radio_rx_timeout();
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
  State = RX_ERROR;
  user_on_radio_rx_error();
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */
static void PrintPayloadHex(const uint8_t *payload, uint16_t size)
{
  char chunk[49];
  uint16_t offset = 0U;

  while (offset < size)
  {
    uint16_t chunk_len = 0U;
    uint16_t i;

    for (i = 0U; (i < 16U) && ((offset + i) < size); i++)
    {
      int written = snprintf(&chunk[chunk_len], sizeof(chunk) - chunk_len, "%02X ", payload[offset + i]);

      if ((written < 0) || ((size_t)written >= (sizeof(chunk) - chunk_len)))
      {
        break;
      }
      chunk_len += (uint16_t)written;
    }
    chunk[chunk_len] = '\0';
    APP_PRINTF("%s", chunk);
    offset += i;
  }
}

static void PrintPayloadString(const uint8_t *payload, uint16_t size)
{
  char chunk[33];
  uint16_t offset = 0U;

  while (offset < size)
  {
    uint16_t chunk_len = 0U;

    while ((chunk_len < (sizeof(chunk) - 1U)) && ((offset + chunk_len) < size))
    {
      uint8_t ch = payload[offset + chunk_len];

      chunk[chunk_len] = (char)(((ch >= 0x20U) && (ch <= 0x7EU)) ? ch : '.');
      chunk_len++;
    }
    chunk[chunk_len] = '\0';
    APP_PRINTF("%s", chunk);
    offset += chunk_len;
  }
}

static uint8_t Ra08Checksum8(const uint8_t *payload, uint16_t size)
{
  uint16_t i;
  uint8_t checksum = 0U;

  if (payload == NULL)
  {
    return 0U;
  }

  for (i = 0U; i < size; i++)
  {
    checksum = (uint8_t)(checksum + payload[i]);
  }

  checksum = (uint8_t)(~checksum + 1U);
  return checksum;
}

static bool ParseRa08TransparentFrame(const uint8_t *frame, uint16_t size,
                                      uint16_t *from_addr, uint16_t *to_addr,
                                      const uint8_t **app_payload, uint16_t *app_size)
{
  if ((frame == NULL) || (size < 5U) || (from_addr == NULL) || (to_addr == NULL) ||
      (app_payload == NULL) || (app_size == NULL))
  {
    return false;
  }

  if (Ra08Checksum8(frame, (uint16_t)(size - 1U)) != frame[size - 1U])
  {
    return false;
  }

  *from_addr = (uint16_t)(((uint16_t)frame[0] << 8) | frame[1]);
  *to_addr = (uint16_t)(((uint16_t)frame[2] << 8) | frame[3]);
  *app_payload = &frame[4];
  *app_size = (uint16_t)(size - 5U);

  return true;
}

/* USER CODE END PrFD */
