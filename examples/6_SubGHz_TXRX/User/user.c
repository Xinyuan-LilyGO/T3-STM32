
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
#include "spi.h"
#include "rtc.h"
#include "adc.h"

/* Private define ------------------------------------------------------------*/

#define TEXT_INFO "# "

#define DIGITAL_SCALE_12BITS             ((uint32_t) 0xFFF)

/* Init variable out of ADC expected conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE    (DIGITAL_SCALE_12BITS + 1)

#define VDDA_APPLI                       ((uint32_t)3300)

/**
  * @brief  Macro to calculate the voltage (unit: mVolt)
  *         corresponding to a ADC conversion data (unit: digital value).
  * @note   ADC measurement data must correspond to a resolution of 12bits
  *         (full scale digital value 4095). If not the case, the data must be
  *         preliminarily rescaled to an equivalent resolution of 12 bits.
  * @note   Analog reference voltage (Vref+) must be known from
  *         user board environment.
  * @param  __VREFANALOG_VOLTAGE__ Analog reference voltage (unit: mV)
  * @param  __ADC_DATA__ ADC conversion data (resolution 12 bits)
  *                       (unit: digital value).
  * @retval ADC conversion data equivalent voltage value (unit: mVolt)
  */
#define __ADC_CALC_DATA_VOLTAGE(__VREFANALOG_VOLTAGE__, __ADC_DATA__)       \
  ((__ADC_DATA__) * (__VREFANALOG_VOLTAGE__) / DIGITAL_SCALE_12BITS)


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

static volatile bool sleep_flag = false;

static int retsd_sta = 0;

/* Variables for ADC conversion data */
static volatile uint16_t   uhADCxConvertedData = VAR_CONVERTED_DATA_INIT_VALUE; /* ADC group regular conversion data */

/* Variables for ADC conversion data computation to physical values */
static volatile uint16_t   uhADCxConvertedData_Voltage_mVolt = 0;  /* Value of voltage calculated from ADC conversion data (unit: mV) */

/* Overwrite functions ---------------------------------------------------------*/
void SystemClock_Decrease(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  /** Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

  /* Disable PLL to reduce power consumption since MSI is used from that point */
  /* Change MSI frequency */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select MSI as system clock source */
  /* Note: Keep AHB and APB prescaler settings from previous structure initialization */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI; 
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

void Entry_SleepMode(void)
{
    LED1_OFF; LED2_OFF;
    HAL_GPIO_DeInit(GPIOA, LED1_Pin | LED2_Pin);

    osThreadSuspend(Thd_LoraTxId);
    osThreadSuspend(Thd_OledId);

    Radio.Sleep();
    
    OLED_WR_Byte(0x8D,OLED_CMD);//电荷泵使能
    OLED_WR_Byte(0x10,OLED_CMD);//关闭电荷泵
    OLED_WR_Byte(0xAE, OLED_CMD); //-- Display OFF
    // HAL_SPI_MspDeInit(&hspi1);
    // HAL_RTC_MspDeInit(&hrtc);

    HAL_SuspendTick();

    HAL_PWREx_EnableFlashPowerDown(PWR_FLASHPD_LPSLEEP);

    /* Reset all RCC Sleep and Stop modes register to */
    /* improve power consumption                      */
    RCC->AHB1SMENR  = 0x0;
    RCC->AHB2SMENR  = 0x0;
    RCC->AHB3SMENR  = 0x0;
    
    RCC->APB1SMENR1 = 0x0;
    RCC->APB1SMENR2 = 0x0;
    RCC->APB2SMENR  = 0x0;

    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();

    /* De-init LED2 */
    // BSP_LED_DeInit(LED2);
    HAL_SPI_MspDeInit(&hspi1);
    HAL_RTC_MspDeInit(&hrtc);
    HAL_ADC_MspInit(&hadc);

    SystemClock_Decrease();

    /* Enter Sleep Mode, wake up is done once User push-button (B1) is pressed */
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BOOT_BTN_Pin)
    {
        LOG_TRACE("BOOT btn is Pressed\n");
        sleep_flag = true;
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

static void Bat_VOT_ADC_Chk(void)
{
    if (HAL_ADC_Start(&hadc) != HAL_OK)
    {
        /* ADC conversion start error */
        Error_Handler();
    }
    if (HAL_ADC_PollForConversion(&hadc, 10) != HAL_OK)
    {
      /* End Of Conversion flag not set on time */
      Error_Handler();
    }
    else
    {
      /* Retrieve ADC conversion data */
      uhADCxConvertedData = HAL_ADC_GetValue(&hadc);
      
      /* Computation of ADC conversions raw data to physical values           */
      /* using helper macro.                                                  */
      uhADCxConvertedData_Voltage_mVolt = __ADC_CALC_DATA_VOLTAGE(VDDA_APPLI, uhADCxConvertedData) * 2;

      LOG_INFO("[%d] Volt:%d.%-2dV\n", uhADCxConvertedData, uhADCxConvertedData_Voltage_mVolt/1000, (uhADCxConvertedData_Voltage_mVolt%1000)/10);
    }
}

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

        Bat_VOT_ADC_Chk();

#if STM32_LORA_MODE_TX
        LED2_TRI;

        // line 2
        snprintf((char *)BufferTx, MAX_APP_BUFFER_SIZE, "%s%d", TEXT_INFO, cnt++);
        snprintf(buf, 16, "TX:      #%d", cnt);
        OLED_ShowString(0, 16, buf, 16, 1);

        // line 4
        snprintf(buf, 32, "SD:%s    Volt:%d.%03d", (retsd_sta == 0 ? " -- " : "PASS"), uhADCxConvertedData_Voltage_mVolt/1000, (uhADCxConvertedData_Voltage_mVolt%1000));
        OLED_ShowString(0, 50, buf, 8, 1);

        LOG_TRACE("%s\n", BufferTx);
        Radio.Send(BufferTx, PAYLOAD_LEN);
#elif STM32_LORA_MODE_RX
        Radio.Rx(RX_TIMEOUT_VALUE);

        // line 4
        snprintf(buf, 32, "SD:%s    Volt:%d.%03d", (retsd_sta == 0 ? " -- " : "PASS"), uhADCxConvertedData_Voltage_mVolt/1000, (uhADCxConvertedData_Voltage_mVolt%1000));
        OLED_ShowString(0, 50, buf, 8, 1);
#endif
        osDelay(1000);
    }
}

static void Oled_ShowPocess(void *arg)
{
    int chk_sd = 0;
    char buf[32];

    retsd_sta = sdcard_init();

    OLED_Init();
#if STM32_LORA_MODE_TX
    // line 1
    OLED_ShowString(40, 0, "LORA-TX ", 16, 1);

    // line 2
    OLED_ShowString(0, 16, "TX:      #", 16, 1);

    // line 3
    snprintf(buf, 32, "Freq:%dM  TPWR:%ddBm", (RF_FREQUENCY / 1000000), TX_OUTPUT_POWER);
    OLED_ShowString(0, 38, buf, 8, 1);

    // line 4
    snprintf(buf, 32, "SD:%s    Volt:%4dV", (retsd_sta == 0 ? " -- " : "PASS"), 0);
    OLED_ShowString(0, 50, buf, 8, 1);

#elif STM32_LORA_MODE_RX

    // line 1
    OLED_ShowString(40, 0, "LORA-RX ", 16, 1);

    // line 2
    snprintf(buf, 32, "RX: %s", BufferRx);
    OLED_ShowString(0, 16, buf, 16, 1);

    // line 3
    snprintf(buf, 32, "Freq:%dM  RSSI:%3ddB", (RF_FREQUENCY / 1000000), RssiValue);
    OLED_ShowString(0, 38, buf, 8, 1);

    // line 4
    snprintf(buf, 32, "SD:%s    Volt:%4dV", (retsd_sta == 0 ? " -- " : "PASS"), 0);
    OLED_ShowString(0, 50, buf, 8, 1);

#endif

    int rand_cnt = 0;

    for (;;)
    {
#if STM32_LORA_MODE_RX
        RxFinishFlag = false;
        // line 2
        snprintf(buf, 16, "RX: %s    ", BufferRx);
        OLED_ShowString(0, 16, buf, 16, 1);

        // line 3
        snprintf(buf, 32, "Freq:%dM  RSSI:%3ddB", (RF_FREQUENCY / 1000000), RssiValue);
        OLED_ShowString(0, 38, buf, 8, 1);
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
    // if (LED_tick++ > 100)
    // {
    //     LED_tick = 0;
    //     LED1_TRI;
    // }

    if(sleep_flag)
    {
        // Entry_SleepMode();
    }
}