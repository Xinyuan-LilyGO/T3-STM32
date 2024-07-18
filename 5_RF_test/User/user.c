
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

    LORA_TX;

    segger_rtt_init("Hello SEGGER!\n");

    OLED_Init();

    OLED_ShowString(40, 0, "RF-TEST", 16, 1);
    OLED_ShowString(0, 32, "Connect UART1", 16, 1);
    OLED_ShowString(0, 48, "Baud: 9600", 16, 1);
    OLED_Refresh();
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

        LED2_TRI;

    }
}