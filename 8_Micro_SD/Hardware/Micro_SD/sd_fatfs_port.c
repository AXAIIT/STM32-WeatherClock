#include <stdint.h>
#include "sd_fatfs_port.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_LowLevelInit(void)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_LowLevelInit(void)
#endif
{
    return 0U;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_BlockRead(uint8_t *buffer, uint32_t sector, uint32_t count)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_BlockRead(uint8_t *buffer, uint32_t sector, uint32_t count)
#endif
{
    (void)buffer;
    (void)sector;
    (void)count;
    return 0U;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_BlockWrite(const uint8_t *buffer, uint32_t sector, uint32_t count)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_BlockWrite(const uint8_t *buffer, uint32_t sector, uint32_t count)
#endif
{
    (void)buffer;
    (void)sector;
    (void)count;
    return 0U;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_GetSectorCount(uint32_t *sector_count)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_GetSectorCount(uint32_t *sector_count)
#endif
{
    (void)sector_count;
    return 0U;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_GetBlockSize(uint32_t *block_size)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_GetBlockSize(uint32_t *block_size)
#endif
{
    (void)block_size;
    return 0U;
}
