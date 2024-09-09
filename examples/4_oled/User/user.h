

#ifndef _USER_H_
#define _USER_H_

#include "main.h"

#define LED1_ON     HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)
#define LED1_OFF    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)
#define LED1_TRI    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, !HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin))
#define LED2_ON     HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET)
#define LED2_OFF    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET)
#define LED2_TRI    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, !HAL_GPIO_ReadPin(LED2_GPIO_Port, LED2_Pin))

#define OLED_CS_0 HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET)
#define OLED_CS_1 HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_SET)
#define OLED_DC_0 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET)
#define OLED_DC_1 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET)

#define OLED_SCK_0 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, GPIO_PIN_5, GPIO_PIN_RESET)
#define OLED_SCK_1 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, GPIO_PIN_5, GPIO_PIN_SET)

#define OLED_SDA_0 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, GPIO_PIN_7, GPIO_PIN_RESET)
#define OLED_SDA_1 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, GPIO_PIN_7, GPIO_PIN_SET)

void user_setup(void);

void user_loop(void);


#endif

