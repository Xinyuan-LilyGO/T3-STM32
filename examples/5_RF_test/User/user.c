
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "oled.h"

uint32_t LED_tick = 0;

void user_setup(void)
{
    LORA_TX; // Select Lora as the sending mode

    OLED_Init();

    OLED_ShowString(40, 0, "RF-TEST", 16, 1);
    OLED_ShowString(0, 16, "AT Slave RF test", 16, 1);
    OLED_ShowString(0, 32, "Connect: UART1", 16, 1);
    OLED_ShowString(0, 48, "Baud:    9600", 16, 1);
    OLED_Refresh();
}

void user_loop(void)
{
    if (LED_tick++ > 100)
    {
        LED_tick = 0;
        LED1_TRI;
    }
}