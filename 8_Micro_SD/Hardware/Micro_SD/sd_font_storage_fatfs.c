#include <stdint.h>
#include <stdio.h>
#include "custom.h"
#include "sd_font_storage_fatfs.h"
#include "sd_fatfs_port.h"

#if defined(__has_include)
    #if __has_include("ff.h")
        #define WEATHERCLOCK_HAS_FATFS_HEADER 1
    #else
        #define WEATHERCLOCK_HAS_FATFS_HEADER 0
    #endif
#elif defined(LV_USE_FS_FATFS) && LV_USE_FS_FATFS
    #define WEATHERCLOCK_HAS_FATFS_HEADER 1
#else
    #define WEATHERCLOCK_HAS_FATFS_HEADER 0
#endif

#if WEATHERCLOCK_HAS_FATFS_HEADER
#include "ff.h"

#define WEATHERCLOCK_FATFS_DRIVE            "0:"
#define WEATHERCLOCK_FONT_FILE_PROBE_MAIN   "0:/font/FONT18.BIN"
#define WEATHERCLOCK_FONT_FILE_PROBE_DAY    "0:/font/FONT24.BIN"

static FATFS g_font_fs;
static uint8_t g_font_fs_mounted = 0U;

uint8_t WeatherClock_SD_FontStorageInit(void)
{
    FRESULT fr;
    FIL file;

    if(g_font_fs_mounted != 0U) {
        return 1U;
    }

    if(WeatherClock_SD_LowLevelInit() == 0U) {
        printf("[FONT] SD low-level init failed\r\n");
        return 0U;
    }

    fr = f_mount(&g_font_fs, WEATHERCLOCK_FATFS_DRIVE, 1);
    if(fr != FR_OK) {
        printf("[FONT] f_mount(%s) failed, fr=%d\r\n", WEATHERCLOCK_FATFS_DRIVE, (int)fr);
        return 0U;
    }

    fr = f_open(&file, WEATHERCLOCK_FONT_FILE_PROBE_MAIN, FA_READ);
    if(fr != FR_OK) {
        FRESULT fr_day = f_open(&file, WEATHERCLOCK_FONT_FILE_PROBE_DAY, FA_READ);
        if(fr_day != FR_OK) {
            printf("[FONT] probe open failed: %s fr=%d; %s fr=%d\r\n",
                   WEATHERCLOCK_FONT_FILE_PROBE_MAIN,
                   (int)fr,
                   WEATHERCLOCK_FONT_FILE_PROBE_DAY,
                   (int)fr_day);
            return 0U;
        }
    }

    (void)f_close(&file);
    g_font_fs_mounted = 1U;
    return 1U;
}

#endif