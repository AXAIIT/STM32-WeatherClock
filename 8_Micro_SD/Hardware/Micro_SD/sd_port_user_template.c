#include <stdint.h>
#include <stdio.h>
#include "sd_port_user_template.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "LCD_spi.h"

#define WEATHERCLOCK_SD_CS_GPIO_CLK   RCC_AHB1Periph_GPIOA
#define WEATHERCLOCK_SD_CS_GPIO_PORT  GPIOA
#define WEATHERCLOCK_SD_CS_GPIO_PIN   GPIO_Pin_4

#define SD_SECTOR_SIZE                512U
#define SD_CARD_TYPE_NONE             0U
#define SD_CARD_TYPE_SDSC             1U
#define SD_CARD_TYPE_SDHC             2U

static uint8_t g_sd_card_type = SD_CARD_TYPE_NONE;
static uint32_t g_sd_sector_count = 0U;
static uint8_t g_sd_init_done = 0U;
static uint8_t g_sd_init_fail_printed = 0U;
static uint8_t g_sd_rw_fail_printed = 0U;
static uint8_t g_sd_last_data_token = 0xFFU;

static void WeatherClock_SD_SetSpiMode0(void)
{
    SPI1->CR1 &= (uint16_t)(~SPI_CR1_SPE);
    SPI1->CR1 &= (uint16_t)(~(SPI_CR1_CPOL | SPI_CR1_CPHA));
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void WeatherClock_SD_SetSpiMode3(void)
{
    SPI1->CR1 &= (uint16_t)(~SPI_CR1_SPE);
    SPI1->CR1 |= (SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void WeatherClock_SD_CsLow(void)
{
    GPIO_ResetBits(WEATHERCLOCK_SD_CS_GPIO_PORT, WEATHERCLOCK_SD_CS_GPIO_PIN);
}

static void WeatherClock_SD_CsHigh(void)
{
    GPIO_SetBits(WEATHERCLOCK_SD_CS_GPIO_PORT, WEATHERCLOCK_SD_CS_GPIO_PIN);
}

static void WeatherClock_SD_CsInit(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_AHB1PeriphClockCmd(WEATHERCLOCK_SD_CS_GPIO_CLK, ENABLE);

    gpio_init.GPIO_Pin = WEATHERCLOCK_SD_CS_GPIO_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(WEATHERCLOCK_SD_CS_GPIO_PORT, &gpio_init);

    WeatherClock_SD_CsHigh();
}

static uint8_t WeatherClock_SD_SpiTxRx(uint8_t data)
{
    return SPI_WriteByte(SPI1, data);
}

static void WeatherClock_SD_Deselect(void)
{
    WeatherClock_SD_CsHigh();
    (void)WeatherClock_SD_SpiTxRx(0xFFU);
}

static uint8_t WeatherClock_SD_WaitReady(uint32_t wait_cycles)
{
    uint8_t resp = 0U;
    while(wait_cycles-- > 0U) {
        resp = WeatherClock_SD_SpiTxRx(0xFFU);
        if(resp == 0xFFU) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t WeatherClock_SD_Select(void)
{
    WeatherClock_SD_CsLow();
    if(WeatherClock_SD_WaitReady(80000U) != 0U) {
        return 1U;
    }
    WeatherClock_SD_Deselect();
    return 0U;
}

static uint8_t WeatherClock_SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t i;
    uint8_t resp;

    if((cmd & 0x80U) != 0U) {
        cmd &= 0x7FU;
        resp = WeatherClock_SD_SendCmd(55U, 0U, 0x01U);
        WeatherClock_SD_Deselect();
        if(resp > 1U) {
            return resp;
        }
    }

    if(WeatherClock_SD_Select() == 0U) {
        return 0xFFU;
    }

    WeatherClock_SD_SpiTxRx((uint8_t)(0x40U | cmd));
    WeatherClock_SD_SpiTxRx((uint8_t)(arg >> 24));
    WeatherClock_SD_SpiTxRx((uint8_t)(arg >> 16));
    WeatherClock_SD_SpiTxRx((uint8_t)(arg >> 8));
    WeatherClock_SD_SpiTxRx((uint8_t)arg);
    WeatherClock_SD_SpiTxRx(crc);

    for(i = 0U; i < 10U; i++) {
        resp = WeatherClock_SD_SpiTxRx(0xFFU);
        if((resp & 0x80U) == 0U) {
            return resp;
        }
    }

    return 0xFFU;
}

static uint8_t WeatherClock_SD_RecvData(uint8_t *buffer, uint16_t len)
{
    uint8_t token;
    uint32_t timeout = 120000U;
    uint16_t i;

    do {
        token = WeatherClock_SD_SpiTxRx(0xFFU);
    } while((token == 0xFFU) && (--timeout > 0U));

    g_sd_last_data_token = token;

    if(token != 0xFEU) {
        return 0U;
    }

    for(i = 0U; i < len; i++) {
        buffer[i] = WeatherClock_SD_SpiTxRx(0xFFU);
    }

    (void)WeatherClock_SD_SpiTxRx(0xFFU);
    (void)WeatherClock_SD_SpiTxRx(0xFFU);
    return 1U;
}

static uint8_t WeatherClock_SD_SendData(const uint8_t *buffer, uint8_t token)
{
    uint16_t i;
    uint8_t resp;

    if(WeatherClock_SD_WaitReady(120000U) == 0U) {
        return 0U;
    }

    WeatherClock_SD_SpiTxRx(token);
    for(i = 0U; i < SD_SECTOR_SIZE; i++) {
        WeatherClock_SD_SpiTxRx(buffer[i]);
    }

    WeatherClock_SD_SpiTxRx(0xFFU);
    WeatherClock_SD_SpiTxRx(0xFFU);

    resp = (uint8_t)(WeatherClock_SD_SpiTxRx(0xFFU) & 0x1FU);
    if(resp != 0x05U) {
        return 0U;
    }

    if(WeatherClock_SD_WaitReady(180000U) == 0U) {
        return 0U;
    }

    return 1U;
}

static uint8_t WeatherClock_SD_DetectCardType(void)
{
    uint8_t i;
    uint8_t r1;
    uint8_t ocr[4];

    WeatherClock_SD_Deselect();
    for(i = 0U; i < 10U; i++) {
        WeatherClock_SD_SpiTxRx(0xFFU);
    }

    r1 = WeatherClock_SD_SendCmd(0U, 0U, 0x95U);
    WeatherClock_SD_Deselect();
    if(r1 != 0x01U) {
        return SD_CARD_TYPE_NONE;
    }

    r1 = WeatherClock_SD_SendCmd(8U, 0x1AAU, 0x87U);
    if(r1 == 0x01U) {
        for(i = 0U; i < 4U; i++) {
            ocr[i] = WeatherClock_SD_SpiTxRx(0xFFU);
        }
        WeatherClock_SD_Deselect();

        if((ocr[2] == 0x01U) && (ocr[3] == 0xAAU)) {
            uint32_t retry = 50000U;
            do {
                r1 = WeatherClock_SD_SendCmd(0x80U | 41U, 0x40000000U, 0x01U);
                WeatherClock_SD_Deselect();
            } while((r1 != 0x00U) && (--retry > 0U));

            if((retry > 0U) && (WeatherClock_SD_SendCmd(58U, 0U, 0x01U) == 0x00U)) {
                for(i = 0U; i < 4U; i++) {
                    ocr[i] = WeatherClock_SD_SpiTxRx(0xFFU);
                }
                WeatherClock_SD_Deselect();
                if((ocr[0] & 0x40U) != 0U) {
                    return SD_CARD_TYPE_SDHC;
                }
                if(WeatherClock_SD_SendCmd(16U, SD_SECTOR_SIZE, 0x01U) != 0x00U) {
                    WeatherClock_SD_Deselect();
                    return SD_CARD_TYPE_NONE;
                }
                WeatherClock_SD_Deselect();
                return SD_CARD_TYPE_SDSC;
            }
        }
    }
    else {
        uint32_t retry = 50000U;
        do {
            r1 = WeatherClock_SD_SendCmd(0x80U | 41U, 0U, 0x01U);
            WeatherClock_SD_Deselect();
        } while((r1 != 0x00U) && (--retry > 0U));

        if(retry == 0U) {
            retry = 50000U;
            do {
                r1 = WeatherClock_SD_SendCmd(1U, 0U, 0x01U);
                WeatherClock_SD_Deselect();
            } while((r1 != 0x00U) && (--retry > 0U));
        }

        if(retry > 0U) {
            if(WeatherClock_SD_SendCmd(16U, SD_SECTOR_SIZE, 0x01U) == 0x00U) {
                WeatherClock_SD_Deselect();
                return SD_CARD_TYPE_SDSC;
            }
            WeatherClock_SD_Deselect();
        }
    }

    WeatherClock_SD_Deselect();
    return SD_CARD_TYPE_NONE;
}

static uint8_t WeatherClock_SD_ReadCsd(uint8_t *csd16)
{
    if(csd16 == 0) {
        return 0U;
    }

    if(WeatherClock_SD_SendCmd(9U, 0U, 0x01U) != 0x00U) {
        WeatherClock_SD_Deselect();
        return 0U;
    }

    if(WeatherClock_SD_RecvData(csd16, 16U) == 0U) {
        WeatherClock_SD_Deselect();
        return 0U;
    }

    WeatherClock_SD_Deselect();
    return 1U;
}

static uint8_t WeatherClock_SD_QuerySectorCount(uint32_t *out_sector_count)
{
    uint8_t csd[16];

    if(out_sector_count == 0) {
        return 0U;
    }

    if(WeatherClock_SD_ReadCsd(csd) == 0U) {
        return 0U;
    }

    if((csd[0] & 0xC0U) == 0x40U) {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3FU) << 16)
                        | ((uint32_t)csd[8] << 8)
                        | (uint32_t)csd[9];
        *out_sector_count = (c_size + 1U) * 1024U;
        return 1U;
    }
    else {
        uint32_t read_bl_len = (uint32_t)(csd[5] & 0x0FU);
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03U) << 10)
                        | ((uint32_t)csd[7] << 2)
                        | ((uint32_t)(csd[8] & 0xC0U) >> 6);
        uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03U) << 1)
                             | ((uint32_t)(csd[10] & 0x80U) >> 7);
        uint32_t blocknr = (c_size + 1U) << (c_size_mult + 2U);
        uint32_t block_len = 1UL << read_bl_len;
        uint64_t capacity = (uint64_t)blocknr * (uint64_t)block_len;
        *out_sector_count = (uint32_t)(capacity / SD_SECTOR_SIZE);
        return 1U;
    }
}

uint8_t WeatherClock_SD_LowLevelInit(void)
{
    if((g_sd_init_done != 0U) && (g_sd_card_type != SD_CARD_TYPE_NONE)) {
        return 1U;
    }

    WeatherClock_SD_CsInit();
    SPI1_Init();
    WeatherClock_SD_SetSpiMode0();

    SPI_SetSpeed(SPI1, 0U);
    g_sd_card_type = WeatherClock_SD_DetectCardType();

    if(g_sd_card_type == SD_CARD_TYPE_NONE) {
        if(g_sd_init_fail_printed == 0U) {
            printf("[SD] init fail: card not detected or no SD_CS wiring\r\n");
            g_sd_init_fail_printed = 1U;
        }
        g_sd_init_done = 0U;
        WeatherClock_SD_SetSpiMode3();
        return 0U;
    }

    SPI_SetSpeed(SPI1, 0U);

    if(WeatherClock_SD_QuerySectorCount(&g_sd_sector_count) == 0U) {
        printf("[SD] init warn: CSD read failed, sector count unknown\r\n");
        g_sd_sector_count = 0U;
    }

    printf("[SD] init ok: type=%u sectors=%lu\r\n", (unsigned int)g_sd_card_type, (unsigned long)g_sd_sector_count);
    g_sd_init_done = 1U;
    g_sd_init_fail_printed = 0U;
    g_sd_rw_fail_printed = 0U;
    WeatherClock_SD_SetSpiMode3();
    return 1U;
}

uint8_t WeatherClock_SD_BlockRead(uint8_t *buffer, uint32_t sector, uint32_t count)
{
    uint8_t retry;
    uint32_t addr;
    uint8_t cmd_resp;

    if((buffer == 0) || (count == 0U) || (g_sd_card_type == SD_CARD_TYPE_NONE)) {
        return 0U;
    }

    WeatherClock_SD_SetSpiMode0();

    while(count-- > 0U) {
        uint8_t ok = 0U;
        for(retry = 0U; retry < 3U; retry++) {
            addr = (g_sd_card_type == SD_CARD_TYPE_SDHC) ? sector : (sector * SD_SECTOR_SIZE);

            cmd_resp = WeatherClock_SD_SendCmd(17U, addr, 0x01U);
            if(cmd_resp != 0x00U) {
                WeatherClock_SD_Deselect();
                continue;
            }

            if(WeatherClock_SD_RecvData(buffer, SD_SECTOR_SIZE) == 0U) {
                WeatherClock_SD_Deselect();
                continue;
            }

            WeatherClock_SD_Deselect();
            ok = 1U;
            break;
        }

        if(ok == 0U) {
            if(g_sd_rw_fail_printed == 0U) {
                printf("[SD] read fail: CMD17 sector=%lu r1=0x%02X token=0x%02X\r\n",
                       (unsigned long)sector,
                       (unsigned int)cmd_resp,
                       (unsigned int)g_sd_last_data_token);
                g_sd_rw_fail_printed = 1U;
            }
            WeatherClock_SD_SetSpiMode3();
            return 0U;
        }

        buffer += SD_SECTOR_SIZE;
        sector++;
    }

    WeatherClock_SD_SetSpiMode3();
    return 1U;
}

uint8_t WeatherClock_SD_BlockWrite(const uint8_t *buffer, uint32_t sector, uint32_t count)
{
    uint8_t retry;
    uint32_t addr;

    if((buffer == 0) || (count == 0U) || (g_sd_card_type == SD_CARD_TYPE_NONE)) {
        return 0U;
    }

    WeatherClock_SD_SetSpiMode0();

    while(count-- > 0U) {
        uint8_t ok = 0U;
        for(retry = 0U; retry < 3U; retry++) {
            addr = (g_sd_card_type == SD_CARD_TYPE_SDHC) ? sector : (sector * SD_SECTOR_SIZE);

            if(WeatherClock_SD_SendCmd(24U, addr, 0x01U) != 0x00U) {
                WeatherClock_SD_Deselect();
                continue;
            }

            if(WeatherClock_SD_SendData(buffer, 0xFEU) == 0U) {
                WeatherClock_SD_Deselect();
                continue;
            }

            WeatherClock_SD_Deselect();
            ok = 1U;
            break;
        }

        if(ok == 0U) {
            if(g_sd_rw_fail_printed == 0U) {
                printf("[SD] write fail: CMD24 sector=%lu\r\n", (unsigned long)sector);
                g_sd_rw_fail_printed = 1U;
            }
            WeatherClock_SD_SetSpiMode3();
            return 0U;
        }

        buffer += SD_SECTOR_SIZE;
        sector++;
    }

    WeatherClock_SD_SetSpiMode3();
    return 1U;
}

uint8_t WeatherClock_SD_GetSectorCount(uint32_t *sector_count)
{
    if(sector_count == 0) {
        return 0U;
    }

    if((g_sd_sector_count == 0U) && (g_sd_card_type != SD_CARD_TYPE_NONE)) {
        if(WeatherClock_SD_QuerySectorCount(&g_sd_sector_count) == 0U) {
            return 0U;
        }
    }

    *sector_count = g_sd_sector_count;
    return 1U;
}

uint8_t WeatherClock_SD_GetBlockSize(uint32_t *block_size)
{
    if(block_size == 0) {
        return 0U;
    }

    *block_size = 1U;
    return 1U;
}
