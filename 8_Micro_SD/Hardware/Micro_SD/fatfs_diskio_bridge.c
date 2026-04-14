#include <stdint.h>
#include "fatfs_diskio_bridge.h"
#include "sd_fatfs_port.h"

#if defined(__has_include)
    #if __has_include("diskio.h")
        #define WEATHERCLOCK_HAS_DISKIO_HEADER 1
    #else
        #define WEATHERCLOCK_HAS_DISKIO_HEADER 0
    #endif
#else
    #define WEATHERCLOCK_HAS_DISKIO_HEADER 0
#endif

#ifndef WEATHERCLOCK_USE_CUSTOM_DISKIO_BRIDGE
    #define WEATHERCLOCK_USE_CUSTOM_DISKIO_BRIDGE 0
#endif

#if WEATHERCLOCK_HAS_DISKIO_HEADER && WEATHERCLOCK_USE_CUSTOM_DISKIO_BRIDGE
#include "diskio.h"

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    return (WeatherClock_SD_LowLevelInit() != 0U) ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    if((buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }
    return (WeatherClock_SD_BlockRead((uint8_t *)buff, (uint32_t)sector, (uint32_t)count) != 0U) ? RES_OK : RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;
    if((buff == 0) || (count == 0U)) {
        return RES_PARERR;
    }
    return (WeatherClock_SD_BlockWrite((const uint8_t *)buff, (uint32_t)sector, (uint32_t)count) != 0U) ? RES_OK : RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;

    if(buff == 0) {
        return RES_PARERR;
    }

    switch(cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            return (WeatherClock_SD_GetSectorCount((uint32_t *)buff) != 0U) ? RES_OK : RES_ERROR;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512U;
            return RES_OK;
        case GET_BLOCK_SIZE:
            return (WeatherClock_SD_GetBlockSize((uint32_t *)buff) != 0U) ? RES_OK : RES_ERROR;
        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    return ((DWORD)(2026 - 1980) << 25)
           | ((DWORD)1 << 21)
           | ((DWORD)1 << 16)
           | ((DWORD)0 << 11)
           | ((DWORD)0 << 5)
           | ((DWORD)0 >> 1);
}

#else

void WeatherClock_FatFsDiskioBridgeStub(void)
{
}

#endif
