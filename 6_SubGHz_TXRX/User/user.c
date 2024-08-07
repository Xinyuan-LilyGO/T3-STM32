
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
#include "gpio.h"

/* Private define ------------------------------------------------------------*/

#define TEXT_INFO "# "

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
uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* Last  Received Buffer Size*/
uint16_t RxBufferSize = 0;
/* Last  Received finish flag*/
bool RxFinishFlag = false;
/* Last  Received packer Rssi*/
int8_t RssiValue = 0;
/* Last  Received packer SNR (in Lora modulation)*/
int8_t SnrValue = 0;

/* App Tx Buffer*/
uint8_t BufferTx[MAX_APP_BUFFER_SIZE];

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
static void Lora_Pocess(void *argument)
{
    UNUSED(argument);
    static int cnt = 0;
    char buf[16];

#if STM32_LORA_MODE_TX
    LORA_TX;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
#elif STM32_LORA_MODE_RX
    LORA_RX;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
#endif

    Lora_init();

    for (;;)
    {

        LED2_TRI;

#if STM32_LORA_MODE_TX
        snprintf((char *)BufferTx, MAX_APP_BUFFER_SIZE, "%s%d", TEXT_INFO, cnt++);
        snprintf(buf, 16, "TX:      #%d", cnt);
        OLED_ShowString(0, 32, buf, 16, 1);

        LOG_TRACE("%s\n", BufferTx);
        Radio.Send(BufferTx, PAYLOAD_LEN);
#elif STM32_LORA_MODE_RX
        Radio.Rx(RX_TIMEOUT_VALUE);
#endif
        osDelay(1000);
    }
}

static void Oled_ShowPocess(void *arg)
{
    int chk_sd = 0;
    char buf[16];

    sdcard_init();

    OLED_Init();
#if STM32_LORA_MODE_TX
    // line 1
    OLED_ShowString(40, 0, "LORA-TX ", 16, 1);

    // line 2
    snprintf(buf, 16, "Freq:    %dMHz", (RF_FREQUENCY / 1000000));
    OLED_ShowString(0, 16, buf, 16, 1);

    // line 3
    OLED_ShowString(0, 32, "TX:      #", 16, 1);

    // line 4
    snprintf(buf, 16, "PWR:     %ddBm", TX_OUTPUT_POWER);
    OLED_ShowString(0, 48, buf, 16, 1);

#elif STM32_LORA_MODE_RX

    // line 1
    OLED_ShowString(40, 0, "LORA-RX ", 16, 1);

    // line 2
    snprintf(buf, 16, "Freq:    %dMHz", (RF_FREQUENCY / 1000000));
    OLED_ShowString(0, 16, buf, 16, 1);

    // line 3
    snprintf(buf, 16, "RX: %s", BufferRx);
    OLED_ShowString(0, 32, buf, 16, 1);

    // line 4
    snprintf(buf, 16, "RSSI:%ddBm", RssiValue);
    OLED_ShowString(0, 48, buf, 16, 1);

#endif

    // snprintf(buf, 16, "SD: %s", sdcard_get_type_str());
    // OLED_ShowString(0, 48, buf, 16, 1);

    for (;;)
    {
#if STM32_LORA_MODE_RX
        RxFinishFlag = false;
        // line 3
        snprintf(buf, 16, "RX: %s", BufferRx);
        OLED_ShowString(0, 32, buf, 16, 1);

        // line 4
        snprintf(buf, 16, "RSSI:%ddBm", RssiValue);
        OLED_ShowString(0, 48, buf, 16, 1);
#endif

        OLED_Refresh();
        osDelay(50);
    }
}

/* Exported functions ---------------------------------------------------------*/
void user_setup(void)
{
    Thd_LoraTxId = osThreadNew(Lora_Pocess, NULL, &Thd_LoraTx_attr);
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