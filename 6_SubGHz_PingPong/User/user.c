
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"
#include "cmsis_os.h"
#include "lib_log.h"
#include "oled.h"
#include "sdcard.h"

/* Private define ------------------------------------------------------------*/

#define TX_OUTPUT_POWER 22

#define TEXT_INFO "#T3_STM32 - "

/*Timeout*/
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

/*Size of the payload to be sent*/
/* Size must be greater of equal the PING and PONG*/
#define MAX_APP_BUFFER_SIZE 255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */

/* External variables ---------------------------------------------------------*/

osThreadId_t Thd_LoraTxId;
const osThreadAttr_t Thd_LoraTx_attr = {
    .name = CFG_TX_PROCESS_NAME,
    .attr_bits = CFG_TX_PROCESS_ATTR_BITS,
    .cb_mem = CFG_TX_PROCESS_CB_MEM,
    .cb_size = CFG_TX_PROCESS_CB_SIZE,
    .stack_mem = CFG_TX_PROCESS_STACK_MEM,
    .priority = CFG_TX_PROCESS_PRIORITY,
    .stack_size = CFG_TX_PROCESS_STACK_SIZE};

osThreadId_t Thd_OledId;
const osThreadAttr_t Thd_Oled_attr = {
    .name = CFG_TX_PROCESS_NAME,
    .attr_bits = CFG_TX_PROCESS_ATTR_BITS,
    .cb_mem = CFG_TX_PROCESS_CB_MEM,
    .cb_size = CFG_TX_PROCESS_CB_SIZE,
    .stack_mem = CFG_TX_PROCESS_STACK_MEM,
    .priority = CFG_TX_PROCESS_PRIORITY,
    .stack_size = CFG_TX_PROCESS_STACK_SIZE + 1};

/* App Rx Buffer*/
static uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* App Tx Buffer*/
static uint8_t BufferTx[MAX_APP_BUFFER_SIZE];

static uint32_t LED_tick = 0;

static bool boot_btn = false;

/* Overwrite functions ---------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BOOT_BTN_Pin)
    {
        // LOG_TRACE("BOOT btn is Pressed\n");
    }
}

/* Static functions ---------------------------------------------------------*/
static void Lora_init(void)
{
    /* Radio Set frequency */
    Radio.SetChannel(RF_FREQUENCY);

    /* Radio configuration */
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
    LOG_INFO("---------------\n\r");
    LOG_INFO("LORA_MODULATION\n\r");
    LOG_INFO("LORA_BW=%d kHz\n\r", (1 << LORA_BANDWIDTH) * 125);
    LOG_INFO("LORA_SF=%d\n\r", LORA_SPREADING_FACTOR);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

    Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);

#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
    APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
    APP_LOG(TS_OFF, VLEVEL_M, "FSK_MODULATION\n\r");
    APP_LOG(TS_OFF, VLEVEL_M, "FSK_BW=%d Hz\n\r", FSK_BANDWIDTH);
    APP_LOG(TS_OFF, VLEVEL_M, "FSK_DR=%d bits/s\n\r", FSK_DATARATE);

    Radio.SetTxConfig(MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                      FSK_DATARATE, 0,
                      FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, 0, TX_TIMEOUT_VALUE);

    Radio.SetRxConfig(MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                      0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                      0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                      0, 0, false, true);

    Radio.SetMaxPayloadLength(MODEM_FSK, MAX_APP_BUFFER_SIZE);

#else
#error "Please define a modulation in the subghz_phy_app.h file."
#endif
    /*fills tx buffer*/
    memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);

    /*starts reception*/
}

bool Boot_GetBtnStatus(void)
{
    bool ret = false;
    if ((boot_btn == true) && (HAL_GPIO_ReadPin(BOOT_BTN_GPIO_Port, BOOT_BTN_Pin) == GPIO_PIN_RESET))
    {
        ret = true;
        boot_btn = false;
    }
    return ret;
}

/* FreeRTOS define ------------------------------------------------------------*/
static void Lora_TxPocess(void *argument)
{
    UNUSED(argument);
    static int cnt = 0;
    char buf[16];

    LORA_TX; // Select Lora as the sending mode
    // LORA_RX;

    Lora_init();

    for (;;)
    {
        snprintf((char *)BufferTx, MAX_APP_BUFFER_SIZE, "%s%d", TEXT_INFO, cnt++);
        LOG_TRACE("%s\n", BufferTx);
        Radio.Send(BufferTx, PAYLOAD_LEN);
        LED2_TRI;

        snprintf(buf, 16, "TX:      #%d", cnt);
        OLED_ShowString(0, 32, buf, 16, 1);
        osDelay(1000);
    }
}

static void Oled_ShowPocess(void *arg)
{
    int chk_sd = 0;
    char buf[16];
 
    sdcard_init();
    
    OLED_Init();
    OLED_ShowString(40, 0, "LORA-TX ", 16, 1);
    OLED_ShowString(0, 16, "Freq:    868MHz", 16, 1);
    OLED_ShowString(0, 32, "TX:      #", 16, 1);
    
    snprintf(buf, 16, "SD: %s", sdcard_get_type_str());
    OLED_ShowString(0, 48, buf, 16, 1);

    for (;;)
    {
        OLED_Refresh();
        osDelay(50);
    }
}

/* Exported functions ---------------------------------------------------------*/
void user_setup(void)
{
    Thd_LoraTxId = osThreadNew(Lora_TxPocess, NULL, &Thd_LoraTx_attr);
    if (Thd_LoraTxId == NULL)
    {
        Error_Handler();
    }

    Thd_OledId = osThreadNew(Oled_ShowPocess, NULL, &Thd_Oled_attr);
    if (Thd_OledId == NULL)
    {
        Error_Handler();
    }
}

void user_loop(void)
{
    if (LED_tick++ > 100)
    {
        LED_tick = 0;
        LED1_TRI;
    }
}