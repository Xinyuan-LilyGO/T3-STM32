
#include "user.h"
#include "usart.h"

#include <string.h>
#include <stdint.h>
#include "lib_log.h"
#include "oled.h"

char txBuffer[] = "Hello World\n";
uint32_t LED_tick = 0;
uint32_t OLED_tick = 0;
uint8_t oled_show = 0;

void user_setup(void)
{
    LED_tick = HAL_GetTick();
    OLED_tick = HAL_GetTick();

    segger_rtt_init("Hello SEGGER!\n");

    OLED_Init();
}

void user_loop(void)
{
    if (HAL_GetTick() - LED_tick > 100)
    {
        LED_tick = HAL_GetTick();
        LED1_TRI;
    }

    if (HAL_GetTick() - OLED_tick > 1000)
    {
        OLED_tick = HAL_GetTick();

        switch (oled_show)
        {
        case 0:
            OLED_ShowString(0, 0, "T3-STM32", 16, 1);
            OLED_ShowString(0, 16, "2024/07/16", 16, 1);
            OLED_Refresh();
            break;
        case 1:
            OLED_ShowString(0, 32, "USART1/USART2", 16, 1);
            OLED_Refresh();
            break;
        case 2:
            OLED_ShowString(0, 48, "Baud: 9600", 16, 1);
            OLED_Refresh();
            break;
        case 3:
            OLED_Clear();
            break;

        default:
            break;
        }

        oled_show++;
        oled_show &= 0x3;

        LED2_TRI;
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)txBuffer, strlen(txBuffer)); // Baud rate: 9600
        HAL_UART_Transmit_DMA(&huart2, (uint8_t *)txBuffer, strlen(txBuffer)); // Baud rate: 9600
        LOG_INFO("(%d) -> %s\n", __LINE__, txBuffer);
    }
}