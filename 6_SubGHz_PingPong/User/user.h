

#ifndef _USER_H_
#define _USER_H_

#include "main.h"

/* Lora TX/RX */
#define LORA_TX     HAL_GPIO_WritePin(LORA_CTRL_GPIO_Port, LORA_CTRL_Pin, GPIO_PIN_SET)
#define LORA_RX     HAL_GPIO_WritePin(LORA_CTRL_GPIO_Port, LORA_CTRL_Pin, GPIO_PIN_RESET)

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

