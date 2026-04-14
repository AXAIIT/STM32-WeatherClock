/*-----------------------------------------------------------------------*/
/* FatFs disk I/O bridge for Weather Clock                               */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "../Micro_SD/sd_fatfs_port.h"

/* 单盘符配置：逻辑盘 0 对应 SD 卡 */
#define DEV_SD_CARD 0U

static volatile DSTATUS s_sd_status = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    if(pdrv != DEV_SD_CARD) {
        return STA_NOINIT;
    }
    return s_sd_status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if(pdrv != DEV_SD_CARD) {
        return STA_NOINIT;
    }

    if(WeatherClock_SD_LowLevelInit() != 0U) {
        s_sd_status = 0;
    }
    else {
        s_sd_status = STA_NOINIT;
    }

    return s_sd_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if((pdrv != DEV_SD_CARD) || (buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }

    if((disk_status(pdrv) & STA_NOINIT) != 0U) {
        return RES_NOTRDY;
    }

    return (WeatherClock_SD_BlockRead((BYTE *)buff, (DWORD)sector, (UINT)count) != 0U) ? RES_OK : RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if((pdrv != DEV_SD_CARD) || (buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }

    if((disk_status(pdrv) & STA_NOINIT) != 0U) {
        return RES_NOTRDY;
    }

    return (WeatherClock_SD_BlockWrite((const BYTE *)buff, (DWORD)sector, (UINT)count) != 0U) ? RES_OK : RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DWORD value = 0U;

    if(pdrv != DEV_SD_CARD) {
        return RES_PARERR;
    }

    switch(cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            if((buff == 0) || (WeatherClock_SD_GetSectorCount(&value) == 0U)) {
                return RES_ERROR;
            }
            *(LBA_t *)buff = (LBA_t)value;
            return RES_OK;

        case GET_SECTOR_SIZE:
            if(buff == 0) {
                return RES_PARERR;
            }
            *(WORD *)buff = 512U;
            return RES_OK;

        case GET_BLOCK_SIZE:
            if((buff == 0) || (WeatherClock_SD_GetBlockSize(&value) == 0U)) {
                return RES_ERROR;
            }
            *(DWORD *)buff = (DWORD)value;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    return ((DWORD)(2026 - 1980) << 25)
           | ((DWORD)1 << 21)
           | ((DWORD)1 << 16);
}

