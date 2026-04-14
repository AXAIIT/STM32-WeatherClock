#ifndef WEATHERCLOCK_SD_FATFS_PORT_H
#define WEATHERCLOCK_SD_FATFS_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t WeatherClock_SD_LowLevelInit(void);
uint8_t WeatherClock_SD_BlockRead(uint8_t *buffer, uint32_t sector, uint32_t count);
uint8_t WeatherClock_SD_BlockWrite(const uint8_t *buffer, uint32_t sector, uint32_t count);
uint8_t WeatherClock_SD_GetSectorCount(uint32_t *sector_count);
uint8_t WeatherClock_SD_GetBlockSize(uint32_t *block_size);

#ifdef __cplusplus
}
#endif

#endif
