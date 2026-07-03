
#ifndef _USER_H_
#define _USER_H_

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#define TX_OUTPUT_POWER 22

/* Timeout */
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

/* Size of the payload to be sent */
#define MAX_APP_BUFFER_SIZE 255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */

#define USER_UART_LINE_MAX_LEN 250U

typedef enum
{
    USER_RADIO_MODE_RX_HEX = 0,
    USER_RADIO_MODE_RX_STRING,
    USER_RADIO_MODE_TX_TRANSPARENT,
} UserRadioMode_t;

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

extern uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
extern uint16_t RxBufferSize;
extern bool RxFinishFlag;
extern int8_t RssiValue;
extern int8_t SnrValue;
extern volatile uint16_t RxFromAddr;
extern volatile uint16_t RxToAddr;

void user_setup(void);
void user_loop(void);

uint16_t user_get_local_addr(void);
bool user_is_rx_string_mode(void);
void user_set_last_rx_addr(uint16_t from_addr, uint16_t to_addr);
void user_on_radio_tx_done(void);
void user_on_radio_tx_timeout(void);
void user_on_radio_rx_timeout(void);
void user_on_radio_rx_error(void);

#endif

