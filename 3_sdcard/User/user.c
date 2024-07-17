#include "main.h"
#include "user.h"
#include "usart.h"
#include "sdcard.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "lib_log.h"

char init_fail[] = "SD Card init fail\n";
char init_pass[] = "SD Card init success\n";
uint32_t LED_tick = 0;
uint32_t SD_tick = 0;
int res;

void user_setup(void)
{
    LED_tick = HAL_GetTick();
    SD_tick = HAL_GetTick();

    segger_rtt_init("Hello SEGGER!\n");

    res = sdcard_init();
    if (res == 0)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)init_fail, strlen(init_fail));
        LOG_INFO("SD Card init fail\n");
    }
    else
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)init_pass, strlen(init_pass));
        LOG_INFO("SD Card init success, type [%s]\n", sdcard_get_type_str());
    }
}

void user_loop(void)
{
    if (HAL_GetTick() - LED_tick > 100)
    {
        LED_tick = HAL_GetTick();
        LED1_TRI;
    }

    if (HAL_GetTick() - SD_tick > 1000)
    {
        SD_tick = HAL_GetTick();

        LED2_TRI;

        res = sdcard_init();
        if (res == 0)
        {
            HAL_UART_Transmit_DMA(&huart1, (uint8_t *)init_fail, strlen(init_fail));
            LOG_INFO("SD Card init fail\n");
        }
        else
        {
            HAL_UART_Transmit_DMA(&huart1, (uint8_t *)init_pass, strlen(init_pass));
            LOG_INFO("SD Card init success, type [%s]\n", sdcard_get_type_str());
        }
    }
}


