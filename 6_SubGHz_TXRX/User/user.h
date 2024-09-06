

#ifndef _USER_H_
#define _USER_H_

#include "main.h"

#define STM32_LORA_MODE_TX (1)
#define STM32_LORA_MODE_RX (!STM32_LORA_MODE_TX)

#define TX_OUTPUT_POWER 22

/*Timeout*/
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

/*Size of the payload to be sent*/
/* Size must be greater of equal the PING and PONG*/
#define MAX_APP_BUFFER_SIZE 255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */


/* Lora TX/RX */
#define LORA_TX     HAL_GPIO_WritePin(LORA_CTRL_GPIO_Port, LORA_CTRL_Pin, GPIO_PIN_RESET)
#define LORA_RX     HAL_GPIO_WritePin(LORA_CTRL_GPIO_Port, LORA_CTRL_Pin, GPIO_PIN_SET)

/* LED */
#define LED1_ON     HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_OFF    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED1_TRI    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin))
#define LED2_ON     HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define LED2_OFF    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)
#define LED2_TRI    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin))

//
void user_setup(void);

void user_loop(void);

#endif

