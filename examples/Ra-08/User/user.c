#include "user.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"
#include "radio_def.h"
#include "cmsis_os.h"
#include "oled.h"
#include "sdcard.h"
#include "gpio.h"
#include "spi.h"
#include "rtc.h"
#include "adc.h"
#include "stm32_adv_trace.h"

/* Private define ------------------------------------------------------------*/

#define DIGITAL_SCALE_12BITS             ((uint32_t)0xFFF)
#define LORA_RX_PREVIEW_BYTES            6U
#define OLED_LINE_CHARS                  21U
#define UART_RX_RING_SIZE                256U
#define TRANSPARENT_FRAME_OVERHEAD       5U

/* Init variable out of ADC expected conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE    (DIGITAL_SCALE_12BITS + 1U)

#define VDDA_APPLI                       ((uint32_t)3300U)

#define USER_EVENT_TX_DONE               (1UL << 0)
#define USER_EVENT_TX_TIMEOUT            (1UL << 1)
#define USER_EVENT_RX_TIMEOUT            (1UL << 2)
#define USER_EVENT_RX_ERROR              (1UL << 3)

/**
  * @brief  Macro to calculate the voltage (unit: mVolt)
  *         corresponding to a ADC conversion data (unit: digital value).
  */
#define __ADC_CALC_DATA_VOLTAGE(__VREFANALOG_VOLTAGE__, __ADC_DATA__)       \
  ((__ADC_DATA__) * (__VREFANALOG_VOLTAGE__) / DIGITAL_SCALE_12BITS)

typedef enum
{
    USER_CONSOLE_MODE_AT = 0,
    USER_CONSOLE_MODE_TRANSPARENT,
} UserConsoleMode_t;

typedef struct
{
    uint32_t frequency;
    uint8_t data_rate;
    uint8_t bandwidth;
    uint8_t code_rate;
    uint8_t power;
    uint8_t iq_inverted;
    uint16_t local_addr;
    uint16_t target_addr;
    UserRadioMode_t radio_mode;
    UserConsoleMode_t console_mode;
    bool tx_busy;
} UserRadioRuntime_t;

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
    .name = "OLED",
    .attr_bits = CFG_TX_PROCESS_ATTR_BITS,
    .cb_mem = CFG_TX_PROCESS_CB_MEM,
    .cb_size = CFG_TX_PROCESS_CB_SIZE,
    .stack_mem = CFG_TX_PROCESS_STACK_MEM,
    .priority = CFG_TX_PROCESS_PRIORITY,
    .stack_size = CFG_TX_PROCESS_STACK_SIZE + 256U};

/* App Rx Buffer */
uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* Last received payload size */
uint16_t RxBufferSize = 0;
/* Last received finish flag */
bool RxFinishFlag = false;
/* Last received packet RSSI */
int8_t RssiValue = 0;
/* Last received packet SNR */
int8_t SnrValue = 0;
/* Last frame addresses */
volatile uint16_t RxFromAddr = 0U;
volatile uint16_t RxToAddr = 0U;

/* App Tx Buffer */
uint8_t BufferTx[MAX_APP_BUFFER_SIZE];

static volatile bool sleep_flag = false;
static int retsd_sta = 0;

/* Variables for ADC conversion data */
static volatile uint16_t uhADCxConvertedData = VAR_CONVERTED_DATA_INIT_VALUE;
static volatile uint16_t uhADCxConvertedData_Voltage_mVolt = 0;

static volatile uint8_t g_uart_rx_ring[UART_RX_RING_SIZE];
static volatile uint16_t g_uart_rx_head = 0U;
static volatile uint16_t g_uart_rx_tail = 0U;
static volatile bool g_uart_rx_overflow = false;
static volatile uint32_t g_user_event_flags = 0U;

static UserRadioRuntime_t g_radio_runtime = {
    .frequency = RF_FREQUENCY,
    .data_rate = (uint8_t)(12U - LORA_SPREADING_FACTOR),
    .bandwidth = LORA_BANDWIDTH,
    .code_rate = LORA_CODINGRATE,
    .power = TX_OUTPUT_POWER,
    .iq_inverted = (uint8_t)LORA_IQ_INVERSION_ON,
    .local_addr = RA08_DEFAULT_LOCAL_ADDR,
    .target_addr = RA08_DEFAULT_TARGET_ADDR,
    .radio_mode = USER_RADIO_MODE_RX_HEX,
    .console_mode = USER_CONSOLE_MODE_AT,
    .tx_busy = false,
};

static char g_console_line[USER_UART_LINE_MAX_LEN + 1U];
static uint16_t g_console_line_len = 0U;
static char g_last_tx_preview[OLED_LINE_CHARS + 1U] = "--";
static char g_last_status[OLED_LINE_CHARS + 1U] = "boot";

/* Static functions ---------------------------------------------------------*/
static void FormatHexPreview(char *out, size_t out_size, const uint8_t *payload, uint16_t payload_size);
static void FormatAsciiPreview(char *out, size_t out_size, const uint8_t *payload, uint16_t payload_size);
static void OLED_ShowLine(uint8_t y, const char *text);
static void SetStatusText(const char *fmt, ...);
static bool IsLoRaParameterValid(uint32_t freq, uint8_t data_rate, uint8_t bandwidth,
                                 uint8_t code_rate, uint8_t power, uint8_t iq_inverted);
static void SetRfPath(UserRadioMode_t mode);
static void ApplyRadioConfiguration(void);
static uint8_t Ra08Checksum8(const uint8_t *payload, uint16_t size);
static void StoreTxTextPreview(const char *text);
static void UartRxUserCallback(uint8_t *rx_char, uint16_t size, uint8_t error);
static bool UartRingPop(uint8_t *out);
static void ProcessConsoleByte(uint8_t ch);
static void ProcessPendingUartInput(void);
static void ProcessConsoleLine(char *line);
static bool ParseUnsignedLong(const char *text, uint32_t *value);
static bool ParseUnsignedShort(const char *text, uint16_t *value);
static uint8_t SplitArgs(char *text, char *argv[], uint8_t max_args);
static bool HandleAtCtx(char *args);
static bool HandleAtCrx(char *args, bool string_mode);
static bool HandleAtLocalAddr(char *args);
static bool HandleAtTargetAddr(char *args);
static void HandleTransparentLine(const char *line);
static const char *GetModeText(void);
static void ProcessPendingRadioEvents(void);
static void UpdateBatteryVoltage(void);
static void DrawOledStatus(void);

/* Overwrite functions ---------------------------------------------------------*/
void SystemClock_Decrease(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Entry_SleepMode(void)
{
    LED1_OFF;
    LED2_OFF;
    HAL_GPIO_DeInit(GPIOA, LED1_Pin | LED2_Pin);

    osThreadSuspend(Thd_LoraTxId);
    osThreadSuspend(Thd_OledId);

    Radio.Sleep();

    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);

    HAL_SuspendTick();
    HAL_PWREx_EnableFlashPowerDown(PWR_FLASHPD_LPSLEEP);

    RCC->AHB1SMENR = 0x0;
    RCC->AHB2SMENR = 0x0;
    RCC->AHB3SMENR = 0x0;
    RCC->APB1SMENR1 = 0x0;
    RCC->APB1SMENR2 = 0x0;
    RCC->APB2SMENR = 0x0;

    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();

    HAL_SPI_MspDeInit(&hspi1);
    HAL_RTC_MspDeInit(&hrtc);
    HAL_ADC_MspInit(&hadc);

    SystemClock_Decrease();
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BOOT_BTN_Pin)
    {
        sleep_flag = true;
    }
}

/* FreeRTOS tasks ------------------------------------------------------------*/
static void Lora_Pocess(void *argument)
{
    uint32_t last_bat_tick = 0U;

    UNUSED(argument);

    ApplyRadioConfiguration();
    (void)UTIL_ADV_TRACE_StartRxProcess(UartRxUserCallback);

    APP_PRINTF("Ra-08 compatible console ready\r\n");
    APP_PRINTF("AT+CADDRSET=<local_addr>\r\n");
    APP_PRINTF("AT+CTXADDRSET=<target_addr>\r\n");
    APP_PRINTF("AT+CRX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>\r\n");
    APP_PRINTF("AT+CRXS=<freq>,<data_rate>,<bandwidth>,<code_rate>,<iqconverted>\r\n");
    APP_PRINTF("AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>,<iqconverted>\r\n");
    SetStatusText("console ready");

    for (;;)
    {
        ProcessPendingUartInput();
        ProcessPendingRadioEvents();

        if ((HAL_GetTick() - last_bat_tick) >= 1000U)
        {
            last_bat_tick = HAL_GetTick();
            UpdateBatteryVoltage();
        }

        osDelay(10);
    }
}

static void Oled_ShowPocess(void *arg)
{
    UNUSED(arg);

    retsd_sta = sdcard_init();
    OLED_Init();
    DrawOledStatus();

    for (;;)
    {
        DrawOledStatus();
        OLED_Refresh();
        osDelay(100);
    }
}

/* Helpers -------------------------------------------------------------------*/
static void FormatHexPreview(char *out, size_t out_size, const uint8_t *payload, uint16_t payload_size)
{
    uint16_t preview_len;
    size_t write_idx = 0U;
    uint16_t i;

    if ((out == NULL) || (out_size == 0U))
    {
        return;
    }

    out[0] = '\0';
    if ((payload == NULL) || (payload_size == 0U))
    {
        return;
    }

    preview_len = (payload_size < LORA_RX_PREVIEW_BYTES) ? payload_size : LORA_RX_PREVIEW_BYTES;

    for (i = 0U; i < preview_len; i++)
    {
        int written = snprintf(&out[write_idx], out_size - write_idx, "%02X", payload[i]);

        if ((written < 0) || ((size_t)written >= (out_size - write_idx)))
        {
            out[out_size - 1U] = '\0';
            return;
        }
        write_idx += (size_t)written;
    }
}

static void FormatAsciiPreview(char *out, size_t out_size, const uint8_t *payload, uint16_t payload_size)
{
    uint16_t preview_len;
    uint16_t i;

    if ((out == NULL) || (out_size == 0U))
    {
        return;
    }

    out[0] = '\0';
    if ((payload == NULL) || (payload_size == 0U))
    {
        return;
    }

    preview_len = (payload_size < (uint16_t)(out_size - 1U)) ? payload_size : (uint16_t)(out_size - 1U);
    for (i = 0U; i < preview_len; i++)
    {
        uint8_t ch = payload[i];

        out[i] = (char)(isprint(ch) ? ch : '.');
    }
    out[preview_len] = '\0';
}

static void OLED_ShowLine(uint8_t y, const char *text)
{
    char line[OLED_LINE_CHARS + 1U];

    (void)snprintf(line, sizeof(line), "%-*.*s",
                   (int)OLED_LINE_CHARS, (int)OLED_LINE_CHARS,
                   (text != NULL) ? text : "");
    OLED_ShowString(0, y, (uint8_t *)line, 8, 1);
}

static void SetStatusText(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void)vsnprintf(g_last_status, sizeof(g_last_status), fmt, args);
    va_end(args);
}

static bool IsLoRaParameterValid(uint32_t freq, uint8_t data_rate, uint8_t bandwidth,
                                 uint8_t code_rate, uint8_t power, uint8_t iq_inverted)
{
    if (data_rate > 7U)
    {
        return false;
    }
    if (bandwidth > 9U)
    {
        return false;
    }
    if ((code_rate < 1U) || (code_rate > 4U))
    {
        return false;
    }
    if (power > 22U)
    {
        return false;
    }
    if (iq_inverted > 1U)
    {
        return false;
    }
    if (!Radio.CheckRfFrequency(freq))
    {
        return false;
    }
    return true;
}

static void SetRfPath(UserRadioMode_t mode)
{
    if (mode == USER_RADIO_MODE_TX_TRANSPARENT)
    {
        LORA_TX;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    }
    else
    {
        LORA_RX;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    }
}

static void ApplyRadioConfiguration(void)
{
    uint32_t spreading_factor = 12U - g_radio_runtime.data_rate;

    SetRfPath(g_radio_runtime.radio_mode);
    Radio.Sleep();
    Radio.SetChannel(g_radio_runtime.frequency);
    Radio.SetTxConfig(MODEM_LORA, (int8_t)g_radio_runtime.power, 0U, g_radio_runtime.bandwidth,
                      spreading_factor, g_radio_runtime.code_rate,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      LORA_CRC_ENABLED, 0U, 0U, g_radio_runtime.iq_inverted, TX_TIMEOUT_VALUE);
    Radio.SetRxConfig(MODEM_LORA, g_radio_runtime.bandwidth, spreading_factor,
                      g_radio_runtime.code_rate, 0U, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0U, LORA_CRC_ENABLED, 0U, 0U, g_radio_runtime.iq_inverted, true);
    Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);
    Radio.SetPublicNetwork(false);
    g_radio_runtime.tx_busy = false;

    if ((g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_HEX) ||
        (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING))
    {
        Radio.Rx(0U);
    }
    else
    {
        Radio.Standby();
    }
}

static uint8_t Ra08Checksum8(const uint8_t *payload, uint16_t size)
{
    uint16_t i;
    uint8_t checksum = 0U;

    if (payload == NULL)
    {
        return 0U;
    }

    for (i = 0U; i < size; i++)
    {
        checksum = (uint8_t)(checksum + payload[i]);
    }

    return (uint8_t)(~checksum + 1U);
}

static void StoreTxTextPreview(const char *text)
{
    if (text == NULL)
    {
        g_last_tx_preview[0] = '-';
        g_last_tx_preview[1] = '-';
        g_last_tx_preview[2] = '\0';
        return;
    }

    (void)snprintf(g_last_tx_preview, sizeof(g_last_tx_preview), "%.*s",
                   (int)(sizeof(g_last_tx_preview) - 1U), text);
}

static void UartRxUserCallback(uint8_t *rx_char, uint16_t size, uint8_t error)
{
    uint16_t next_head;

    if ((error != 0U) || (rx_char == NULL) || (size == 0U))
    {
        return;
    }

    next_head = (uint16_t)((g_uart_rx_head + 1U) % UART_RX_RING_SIZE);
    if (next_head == g_uart_rx_tail)
    {
        g_uart_rx_overflow = true;
        return;
    }

    g_uart_rx_ring[g_uart_rx_head] = rx_char[0];
    g_uart_rx_head = next_head;
}

static bool UartRingPop(uint8_t *out)
{
    if ((out == NULL) || (g_uart_rx_tail == g_uart_rx_head))
    {
        return false;
    }

    *out = g_uart_rx_ring[g_uart_rx_tail];
    g_uart_rx_tail = (uint16_t)((g_uart_rx_tail + 1U) % UART_RX_RING_SIZE);
    return true;
}

static void ProcessConsoleByte(uint8_t ch)
{
    if ((ch == '\r') || (ch == '\n'))
    {
        if (g_console_line_len > 0U)
        {
            g_console_line[g_console_line_len] = '\0';
            ProcessConsoleLine(g_console_line);
            g_console_line_len = 0U;
        }
        return;
    }

    if (g_console_line_len < USER_UART_LINE_MAX_LEN)
    {
        g_console_line[g_console_line_len++] = (char)ch;
    }
    else
    {
        g_console_line_len = 0U;
        SetStatusText("line too long");
        APP_PRINTF("\r\n+CME ERROR:1\r\n");
    }
}

static void ProcessPendingUartInput(void)
{
    uint8_t ch;

    if (g_uart_rx_overflow)
    {
        g_uart_rx_overflow = false;
        SetStatusText("uart overflow");
    }

    while (UartRingPop(&ch))
    {
        ProcessConsoleByte(ch);
    }
}

static void ProcessConsoleLine(char *line)
{
    if ((line == NULL) || (line[0] == '\0'))
    {
        return;
    }

    if (g_radio_runtime.console_mode == USER_CONSOLE_MODE_TRANSPARENT)
    {
        HandleTransparentLine(line);
        return;
    }

    if (strcmp(line, "AT") == 0)
    {
        APP_PRINTF("OK\r\n");
        SetStatusText("AT ok");
        return;
    }

    if (strncmp(line, "AT+CADDRSET=", 12) == 0)
    {
        if (HandleAtLocalAddr(&line[12]))
        {
            return;
        }
    }
    else if (strncmp(line, "AT+CTXADDRSET=", 14) == 0)
    {
        if (HandleAtTargetAddr(&line[14]))
        {
            return;
        }
    }
    else if (strncmp(line, "AT+CRX=", 7) == 0)
    {
        if (HandleAtCrx(&line[7], false))
        {
            return;
        }
    }
    else if (strncmp(line, "AT+CRXS=", 8) == 0)
    {
        if (HandleAtCrx(&line[8], true))
        {
            return;
        }
    }
    else if (strncmp(line, "AT+CTX=", 7) == 0)
    {
        if (HandleAtCtx(&line[7]))
        {
            return;
        }
    }
    else if (strncmp(line, "AT+CSLEEP=", 10) == 0)
    {
        APP_PRINTF("AT+CSLEEP is not supported in this demo\r\n");
        SetStatusText("CSLEEP skipped");
        return;
    }

    APP_PRINTF("\r\n+CME ERROR:1\r\n");
    SetStatusText("invalid cmd");
}

static bool ParseUnsignedLong(const char *text, uint32_t *value)
{
    char *end_ptr = NULL;
    unsigned long parsed;

    if ((text == NULL) || (value == NULL) || (text[0] == '\0'))
    {
        return false;
    }

    parsed = strtoul(text, &end_ptr, 0);
    if ((end_ptr == NULL) || (*end_ptr != '\0'))
    {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

static bool ParseUnsignedShort(const char *text, uint16_t *value)
{
    uint32_t parsed = 0U;

    if (!ParseUnsignedLong(text, &parsed) || (parsed > 0xFFFFU))
    {
        return false;
    }

    *value = (uint16_t)parsed;
    return true;
}

static uint8_t SplitArgs(char *text, char *argv[], uint8_t max_args)
{
    uint8_t argc = 0U;
    char *token;

    if ((text == NULL) || (argv == NULL) || (max_args == 0U))
    {
        return 0U;
    }

    token = strtok(text, ",");
    while ((token != NULL) && (argc < max_args))
    {
        argv[argc++] = token;
        token = strtok(NULL, ",");
    }

    return argc;
}

static bool HandleAtCtx(char *args)
{
    char *argv[6];
    uint8_t argc;
    uint32_t freq;
    uint32_t data_rate;
    uint32_t bandwidth;
    uint32_t code_rate;
    uint32_t power;
    uint32_t iq_inverted = 0U;

    argc = SplitArgs(args, argv, 6U);
    if ((argc != 5U) && (argc != 6U))
    {
        return false;
    }

    if (!ParseUnsignedLong(argv[0], &freq) ||
        !ParseUnsignedLong(argv[1], &data_rate) ||
        !ParseUnsignedLong(argv[2], &bandwidth) ||
        !ParseUnsignedLong(argv[3], &code_rate) ||
        !ParseUnsignedLong(argv[4], &power))
    {
        return false;
    }
    if ((argc == 6U) && !ParseUnsignedLong(argv[5], &iq_inverted))
    {
        return false;
    }

    if (!IsLoRaParameterValid(freq, (uint8_t)data_rate, (uint8_t)bandwidth,
                              (uint8_t)code_rate, (uint8_t)power, (uint8_t)iq_inverted))
    {
        return false;
    }

    g_radio_runtime.frequency = freq;
    g_radio_runtime.data_rate = (uint8_t)data_rate;
    g_radio_runtime.bandwidth = (uint8_t)bandwidth;
    g_radio_runtime.code_rate = (uint8_t)code_rate;
    g_radio_runtime.power = (uint8_t)power;
    g_radio_runtime.iq_inverted = (uint8_t)iq_inverted;
    g_radio_runtime.radio_mode = USER_RADIO_MODE_TX_TRANSPARENT;
    g_radio_runtime.console_mode = USER_CONSOLE_MODE_TRANSPARENT;
    ApplyRadioConfiguration();

    APP_PRINTF("config radio params data(freq: %lu, dr: %u, bw:%u, cr: %u, power: %u)\r\n",
               (unsigned long)g_radio_runtime.frequency,
               (unsigned int)g_radio_runtime.data_rate,
               (unsigned int)g_radio_runtime.bandwidth,
               (unsigned int)g_radio_runtime.code_rate,
               (unsigned int)g_radio_runtime.power);
    APP_PRINTF(">");
    SetStatusText("transparent tx");
    StoreTxTextPreview(">");
    return true;
}

static bool HandleAtCrx(char *args, bool string_mode)
{
    char *argv[5];
    uint8_t argc;
    uint32_t freq;
    uint32_t data_rate;
    uint32_t bandwidth;
    uint32_t code_rate;
    uint32_t iq_inverted = 0U;

    argc = SplitArgs(args, argv, 5U);
    if ((argc != 4U) && (argc != 5U))
    {
        return false;
    }

    if (!ParseUnsignedLong(argv[0], &freq) ||
        !ParseUnsignedLong(argv[1], &data_rate) ||
        !ParseUnsignedLong(argv[2], &bandwidth) ||
        !ParseUnsignedLong(argv[3], &code_rate))
    {
        return false;
    }
    if ((argc == 5U) && !ParseUnsignedLong(argv[4], &iq_inverted))
    {
        return false;
    }

    if (!IsLoRaParameterValid(freq, (uint8_t)data_rate, (uint8_t)bandwidth,
                              (uint8_t)code_rate, g_radio_runtime.power, (uint8_t)iq_inverted))
    {
        return false;
    }

    g_radio_runtime.frequency = freq;
    g_radio_runtime.data_rate = (uint8_t)data_rate;
    g_radio_runtime.bandwidth = (uint8_t)bandwidth;
    g_radio_runtime.code_rate = (uint8_t)code_rate;
    g_radio_runtime.iq_inverted = (uint8_t)iq_inverted;
    g_radio_runtime.radio_mode = string_mode ? USER_RADIO_MODE_RX_STRING : USER_RADIO_MODE_RX_HEX;
    g_radio_runtime.console_mode = USER_CONSOLE_MODE_AT;
    ApplyRadioConfiguration();

    APP_PRINTF("params num: %u ,start to recv package (freq: %lu, dr:%u, bw: %u, cr: %u)\r\n",
               (unsigned int)argc,
               (unsigned long)g_radio_runtime.frequency,
               (unsigned int)g_radio_runtime.data_rate,
               (unsigned int)g_radio_runtime.bandwidth,
               (unsigned int)g_radio_runtime.code_rate);
    SetStatusText(string_mode ? "rx string" : "rx hex");
    return true;
}

static bool HandleAtLocalAddr(char *args)
{
    uint16_t local_addr = 0U;

    if (!ParseUnsignedShort(args, &local_addr))
    {
        return false;
    }

    g_radio_runtime.local_addr = local_addr;
    APP_PRINTF("set local address: %u \r\n", (unsigned int)g_radio_runtime.local_addr);
    SetStatusText("local=%u", (unsigned int)g_radio_runtime.local_addr);
    return true;
}

static bool HandleAtTargetAddr(char *args)
{
    uint16_t target_addr = 0U;

    if (!ParseUnsignedShort(args, &target_addr))
    {
        return false;
    }

    g_radio_runtime.target_addr = target_addr;
    APP_PRINTF("set target address: %u \r\n", (unsigned int)g_radio_runtime.target_addr);
    SetStatusText("target=%u", (unsigned int)g_radio_runtime.target_addr);
    return true;
}

static void HandleTransparentLine(const char *line)
{
    uint16_t text_len;
    uint16_t frame_len;
    radio_status_t status;

    if (strcmp(line, "+++") == 0)
    {
        g_radio_runtime.console_mode = USER_CONSOLE_MODE_AT;
        APP_PRINTF("Quit transparent\r\n");
        SetStatusText("quit transparent");
        return;
    }

    text_len = (uint16_t)strlen(line);
    frame_len = (uint16_t)(text_len + TRANSPARENT_FRAME_OVERHEAD);
    if (text_len == 0U)
    {
        APP_PRINTF(">");
        return;
    }
    if (frame_len > MAX_APP_BUFFER_SIZE)
    {
        APP_PRINTF("\r\n+CME ERROR:1\r\n");
        SetStatusText("payload too long");
        return;
    }
    if (g_radio_runtime.tx_busy)
    {
        APP_PRINTF("TX busy\r\n>");
        SetStatusText("TX busy");
        return;
    }

    BufferTx[0] = (uint8_t)((g_radio_runtime.local_addr >> 8) & 0xFFU);
    BufferTx[1] = (uint8_t)(g_radio_runtime.local_addr & 0xFFU);
    BufferTx[2] = (uint8_t)((g_radio_runtime.target_addr >> 8) & 0xFFU);
    BufferTx[3] = (uint8_t)(g_radio_runtime.target_addr & 0xFFU);
    (void)memcpy(&BufferTx[4], line, text_len);
    BufferTx[frame_len - 1U] = Ra08Checksum8(BufferTx, (uint16_t)(frame_len - 1U));

    SetRfPath(USER_RADIO_MODE_TX_TRANSPARENT);
    g_radio_runtime.tx_busy = true;
    status = Radio.Send(BufferTx, (uint8_t)frame_len);
    if (status != RADIO_STATUS_OK)
    {
        g_radio_runtime.tx_busy = false;
        APP_PRINTF("\r\n+CME ERROR:1\r\n");
        SetStatusText("TX send err");
        return;
    }

    APP_PRINTF("send data: ");
    for (uint16_t i = 0U; i < frame_len; i++)
    {
        APP_PRINTF("%02X ", BufferTx[i]);
    }
    APP_PRINTF("\r\n>");
    StoreTxTextPreview(line);
    SetStatusText("TX queued");
}

static const char *GetModeText(void)
{
    if (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING)
    {
        return "MODE:CRXS";
    }
    if (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_HEX)
    {
        return "MODE:CRX";
    }
    if (g_radio_runtime.console_mode == USER_CONSOLE_MODE_TRANSPARENT)
    {
        return "MODE:CTX";
    }
    return "MODE:AT-TX";
}

static void ProcessPendingRadioEvents(void)
{
    uint32_t flags = g_user_event_flags;

    if (flags == 0U)
    {
        return;
    }

    g_user_event_flags = 0U;

    if ((flags & USER_EVENT_TX_DONE) != 0U)
    {
        g_radio_runtime.tx_busy = false;
        LED2_TRI;
        SetStatusText("TX done");
    }

    if ((flags & USER_EVENT_TX_TIMEOUT) != 0U)
    {
        g_radio_runtime.tx_busy = false;
        SetStatusText("TX timeout");
    }

    if ((flags & USER_EVENT_RX_TIMEOUT) != 0U)
    {
        SetStatusText("RX timeout");
        if ((g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_HEX) ||
            (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING))
        {
            Radio.Rx(0U);
        }
    }

    if ((flags & USER_EVENT_RX_ERROR) != 0U)
    {
        SetStatusText("RX error");
        if ((g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_HEX) ||
            (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING))
        {
            Radio.Rx(0U);
        }
    }
}

static void UpdateBatteryVoltage(void)
{
    if (HAL_ADC_Start(&hadc) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_ADC_PollForConversion(&hadc, 10U) != HAL_OK)
    {
        Error_Handler();
    }

    uhADCxConvertedData = HAL_ADC_GetValue(&hadc);
    uhADCxConvertedData_Voltage_mVolt =
        (uint16_t)(__ADC_CALC_DATA_VOLTAGE(VDDA_APPLI, uhADCxConvertedData) * 2U);
}

static void DrawOledStatus(void)
{
    char line[32];
    char data_preview[OLED_LINE_CHARS + 1U];

    OLED_ShowLine(0, GetModeText());

    (void)snprintf(line, sizeof(line), "F:%lu", (unsigned long)g_radio_runtime.frequency);
    OLED_ShowLine(8, line);

    (void)snprintf(line, sizeof(line), "SF%u BW%u CR%u P%u",
                   (unsigned int)(12U - g_radio_runtime.data_rate),
                   (unsigned int)g_radio_runtime.bandwidth,
                   (unsigned int)g_radio_runtime.code_rate,
                   (unsigned int)g_radio_runtime.power);
    OLED_ShowLine(16, line);

    (void)snprintf(line, sizeof(line), "IQ%u CRC%u PRE%u",
                   (unsigned int)g_radio_runtime.iq_inverted,
                   (unsigned int)LORA_CRC_ENABLED,
                   (unsigned int)LORA_PREAMBLE_LENGTH);
    OLED_ShowLine(24, line);

    (void)snprintf(line, sizeof(line), "L:%u T:%u",
                   (unsigned int)g_radio_runtime.local_addr,
                   (unsigned int)g_radio_runtime.target_addr);
    OLED_ShowLine(32, line);

    if ((g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING) && (RxBufferSize > 0U))
    {
        FormatAsciiPreview(data_preview, sizeof(data_preview), BufferRx, RxBufferSize);
        (void)snprintf(line, sizeof(line), "RX:%s", data_preview);
    }
    else if ((g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_HEX) && (RxBufferSize > 0U))
    {
        FormatHexPreview(data_preview, sizeof(data_preview), BufferRx, RxBufferSize);
        (void)snprintf(line, sizeof(line), "RX:%s", data_preview);
    }
    else
    {
        (void)snprintf(line, sizeof(line), "TX:%s", g_last_tx_preview);
    }
    OLED_ShowLine(40, line);

    (void)snprintf(line, sizeof(line), "SRC:%u R:%d V:%u.%02u",
                   (unsigned int)RxFromAddr,
                   (int)RssiValue,
                   (unsigned int)(uhADCxConvertedData_Voltage_mVolt / 1000U),
                   (unsigned int)((uhADCxConvertedData_Voltage_mVolt % 1000U) / 10U));
    OLED_ShowLine(48, line);

    (void)snprintf(line, sizeof(line), "ST:%s", g_last_status);
    OLED_ShowLine(56, line);
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
    if (sleep_flag)
    {
        sleep_flag = false;
        SetStatusText("sleep req");
        /* Entry_SleepMode(); */
    }
}

uint16_t user_get_local_addr(void)
{
    return g_radio_runtime.local_addr;
}

bool user_is_rx_string_mode(void)
{
    return (g_radio_runtime.radio_mode == USER_RADIO_MODE_RX_STRING);
}

void user_set_last_rx_addr(uint16_t from_addr, uint16_t to_addr)
{
    RxFromAddr = from_addr;
    RxToAddr = to_addr;
}

void user_on_radio_tx_done(void)
{
    g_user_event_flags |= USER_EVENT_TX_DONE;
}

void user_on_radio_tx_timeout(void)
{
    g_user_event_flags |= USER_EVENT_TX_TIMEOUT;
}

void user_on_radio_rx_timeout(void)
{
    g_user_event_flags |= USER_EVENT_RX_TIMEOUT;
}

void user_on_radio_rx_error(void)
{
    g_user_event_flags |= USER_EVENT_RX_ERROR;
}
