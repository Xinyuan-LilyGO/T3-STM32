
#include "sdcard.h"
#include "stm32wlxx_hal.h"
#include "user.h"
#include "spi.h"
#include "lib_log.h"
#include <stdio.h>

static uint8_t CardType;
static uint8_t PowerFlag = 0;       /* Power flag */
/***************************************
 * SPI functions
 **************************************/

/* slave select */
static void select(void)  
{
    SDCARD_CS_0;
    HAL_Delay(1);
}

/* slave deselect */
static void deselect(void)
{
    SDCARD_CS_1;
    HAL_Delay(1);
}

/* SPI transmit a byte */
static void spi_tx_btye(uint8_t data)
{
    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, HSPI_TIMEOUT);
}

/* SPI transmit buffer */
static void spi_tx_buff(uint8_t *buff, uint16_t len)
{
    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_Transmit(HSPI_SDCARD, buff, len, HSPI_TIMEOUT);
}

/* SPI receive a byte */
static uint8_t spi_rx_btye(void)
{
    uint8_t dummy, data;
    dummy = 0xff;

    while(!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE));
    HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, HSPI_TIMEOUT);

    return data;
}

/* SPI receive a byte via pointer */
static void spi_rx_buff(uint8_t *buff)
{
    *buff = spi_rx_btye();
}

/***************************************
 * SD functions
 **************************************/

/* wait SD ready */
static uint8_t sd_ready(void)
{
    uint8_t res;
    uint32_t try_cnt = 500;

    /* if SD goes ready, receives 0xFF */
    do{
        res = spi_rx_btye();
    } while ((res != 0xFF) && (try_cnt--));

    return res;
}

/* power on */
static void sd_power_on(void)
{
    uint8_t args[6];
    uint32_t cnt = 0x1fff;

    deselect(); /* transmit bytes to wake up */
    for(int i = 0; i < 10; i++) 
    {
        spi_tx_btye(0xff);
    }
    select();   /* slave select */

    /* make idle state */
    args[0] = CMD0;   /* CMD0:GO_IDLE_STATE */
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    args[4] = 0;
    args[5] = 0x95;   /* CRC */

    spi_tx_buff(args, sizeof(args));

    /* wait response */
    while ((spi_rx_btye() != 0x01) && cnt)
    {
        cnt--;
    }
    deselect();
    spi_tx_btye(0xff);

    PowerFlag = 1;
}

/* power off */
void sd_power_off(void)
{
    PowerFlag = 0;
}
/* check power flag */

/* receive data block */

/* transmit data block */

/* transmit command */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t crc, res;

    /* wait SD ready */
    if(sd_ready() != 0xff) return 0xff;

    /* prepare CRC */
    if(cmd == CMD0)      crc = 0x95;  /* CRC for CMD0(0) */
    else if(cmd == CMD8) crc = 0x87;  /* CRC for CMD8(0x1AA) */
    else crc = 1;

    /* transmit command */
    spi_tx_btye(cmd);
    spi_tx_btye((uint8_t)(arg >> 24));  /* Argument[31..24] */
    spi_tx_btye((uint8_t)(arg >> 16));  /* Argument[23..16] */
    spi_tx_btye((uint8_t)(arg >> 8));   /* Argument[15..8] */
    spi_tx_btye((uint8_t)arg);          /* Argument[7..0] */
    spi_tx_btye(crc);

    /* Skip a stuff byte when STOP_TRANSMISSION */
    if(cmd == CMD12) spi_rx_btye();

    /* receive response */
    uint8_t n = 10;
    do {
        res = spi_rx_btye();
    } while ((res & 0x80) && --n);

    return res;
}

/***************************************
 * user_diskio.c functions
 **************************************/
const char * sdcard_get_type_str(void)
{
    static char str[16];
    switch (CardType)
    {
    case 0x0c:
        snprintf(str, 16, "SDHC v2");
        break;
    case 0x04:
        snprintf(str, 16, "SDSC v2");
        break;
    case 0x02:
        snprintf(str, 16, "SDSC v1");
        break;
    case 0x01:
        snprintf(str, 16, "MMC");
        break;
    case 0x00:
        snprintf(str, 16, "NO Find SD!");
        break;
    default:
        break;
    }
    return str;
}

int sdcard_init(void)
{
    uint32_t try_cnt = 1000;
    uint8_t n, type, ocr[4];

    /* power on */
    sd_power_on();

    /* slave select */
    select();

    /* check disk type */
    type = 0;

    /* send GO_IDLE_STATE command */
    if(sd_send_cmd(CMD0, 0) == 1) 
    {
        /* SDC V2+ accept CMD8 command, http://elm-chan.org/docs/mmc/mmc_e.html */
        if (sd_send_cmd(CMD8, 0x1AA) == 1)
        {
            /* operation condition register */
            for(n = 0; n < 4; n++) {
                ocr[n] = spi_rx_btye();
            }

            /* voltage range 2.7-3.6V */
            if(ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                /* ACMD41 with HCS bit */
                do{
                    if(sd_send_cmd(CMD55, 0) <= 1 && sd_send_cmd(CMD41, 1UL << 30) == 0)
                        break;
                } while (try_cnt--);

                /* READ_OCR */
                if((try_cnt) && (sd_send_cmd(CMD58, 0) == 0))
                {
                    for(n = 0; n < 4; n++) {
                        ocr[n] = spi_rx_btye();
                    }
                    LOG_BUFF("CMD58", ocr, 4);
                    /* SDv2 (HC or SC) */
                    type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;


                    if(ocr[0] & 0x40) {
                        LOG_INFO("(%d) type: SDv2 HC\n", __LINE__);
                    } else {
                        LOG_INFO("(%d) type: SDv2 SC\n", __LINE__);
                    }
                    
                }
            }
        }
        else 
        {
            /* SDC V1 or MMC */
            type = (sd_send_cmd(CMD55, 0) <= 1) && (sd_send_cmd(CMD41, 0) <= 1) ? CT_SD1 : CT_MMC;

            do{
                if(type == CT_SD1){
                    if(sd_send_cmd(CMD55, 0) <= 1 && sd_send_cmd(CMD41, 0) == 0) 
                        break;
                }
                else {
                    if(sd_send_cmd(CMD1, 0 == 0))
                        break;
                }

            } while (try_cnt--);
        }
    }
    CardType = type;
    LOG_USER("CardType = 0x%x\n", CardType);
    /* Idle */
    deselect();
    spi_rx_btye();

    if(type) {
        /* Initialization success */

    }else{
        /* Initialization failed */
        sd_power_off();
    }
    return type;
}
