/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*********************
 *      INCLUDES
 *********************/
/* 标准输入输出：用于 vsnprintf 格式化字符串 */
#include <stdio.h>
/* 可变参数支持：用于实现类似 printf 的接口 */
#include <stdarg.h>
#include <stdint.h>
/* LVGL 主头文件 */
#include "lvgl.h"
#include "lv_font_loader.h"
#include "lv_fsdrv.h"
/* 本模块头文件（声明自定义控件与辅助函数） */
#include "custom.h"
#include "sd_font_storage_fatfs.h"

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

LV_FONT_DECLARE(lv_font_WeekdayCN_Subset_30);

extern int screen_1_date_year_init_value;
extern int screen_1_date_month_init_value;
extern int screen_1_date_day_init_value;

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_span_t *screen_1_weekday_day_span = NULL;
static lv_font_t *g_sd_font_18 = NULL;
static lv_font_t *g_sd_font_24 = NULL;
static uint8_t g_sd_font_load_done = 0U;
static uint8_t g_sd_font_18_cjk_ok = 0U;
static uint8_t g_sd_font_24_cjk_ok = 0U;
static uint8_t g_sd_font_apply_error_log_done = 0U;

#define WEATHERCLOCK_FORCE_WEATHER_USE_SD18 0

static const char *week_day_suffix[7] = {
    "\xE4\xB8\x80", /* 一 */
    "\xE4\xBA\x8C", /* 二 */
    "\xE4\xB8\x89", /* 三 */
    "\xE5\x9B\x9B", /* 四 */
    "\xE4\xBA\x94", /* 五 */
    "\xE5\x85\xAD", /* 六 */
    "\xE6\x97\xA5"  /* 日 */
};

static uint32_t custom_get_file_size(const char *path)
{
    lv_fs_file_t file;
    uint32_t size = 0U;

    if(path == NULL) {
        return 0U;
    }

    if(lv_fs_open(&file, path, LV_FS_MODE_RD) != LV_FS_RES_OK) {
        return 0U;
    }

    if(lv_fs_seek(&file, 0U, LV_FS_SEEK_END) == LV_FS_RES_OK) {
        (void)lv_fs_tell(&file, &size);
    }

    (void)lv_fs_close(&file);
    return size;
}

static uint8_t custom_font_has_codepoints(const lv_font_t *font, const uint32_t *codepoints, uint16_t count, const char *tag)
{
    uint32_t codepoint;
    uint16_t i;
    lv_font_glyph_dsc_t dsc;
    const uint8_t *bitmap;

    if((font == NULL) || (codepoints == NULL) || (count == 0U)) {
        return 0U;
    }

    for(i = 0U; i < count; i++) {
        codepoint = codepoints[i];
        if(lv_font_get_glyph_dsc(font, &dsc, codepoint, 0U) == false) {
            printf("[FONT] %s miss glyph dsc: U+%04lX\r\n", tag, (unsigned long)codepoint);
            return 0U;
        }

        bitmap = lv_font_get_glyph_bitmap(font, codepoint);
        if(bitmap == NULL) {
            printf("[FONT] %s miss glyph bmp: U+%04lX\r\n", tag, (unsigned long)codepoint);
            return 0U;
        }
    }

    return 1U;
}

static const uint32_t k_weather_font_cp_18[] = {
    0x591A, 0x4E91, 0x6674, 0x9634, 0x5C0F, 0x4E2D, 0x5927, 0x96E8,
    0x66B4, 0x96F7, 0x9635, 0x96EA, 0x96FE, 0x973E, 0x5F3A, 0x6D53,
    0x8F7B, 0x7279, 0x8F6C, 0x5BA4, 0x5185, 0x5916, 0x661F, 0x671F,
    0x4E00, 0x4E8C, 0x4E09, 0x56DB, 0x4E94, 0x516D, 0x65E5,
    0x002D, 0x0025, 0x2103,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034,
    0x0035, 0x0036, 0x0037, 0x0038, 0x0039
};

static const uint32_t k_weather_font_cp_24[] = {
    0x591A, 0x4E91, 0x6674, 0x9634, 0x5C0F, 0x4E2D, 0x5927, 0x96E8,
    0x66B4, 0x96F7, 0x9635, 0x96EA, 0x96FE, 0x973E, 0x5F3A, 0x6D53,
    0x8F7B, 0x7279, 0x8F6C
};

static uint32_t custom_utf8_next_codepoint(const char *text, uint32_t *index)
{
    const uint8_t *bytes;

    if((text == NULL) || (index == NULL)) {
        return 0U;
    }

    bytes = (const uint8_t *)&text[*index];
    if(bytes[0] == 0U) {
        return 0U;
    }

    if(bytes[0] < 0x80U) {
        *index += 1U;
        return (uint32_t)bytes[0];
    }

    if((bytes[0] & 0xE0U) == 0xC0U) {
        if((bytes[1] & 0xC0U) != 0x80U) {
            *index += 1U;
            return 0U;
        }
        *index += 2U;
        return (uint32_t)(((bytes[0] & 0x1FU) << 6) | (bytes[1] & 0x3FU));
    }

    if((bytes[0] & 0xF0U) == 0xE0U) {
        if(((bytes[1] & 0xC0U) != 0x80U) || ((bytes[2] & 0xC0U) != 0x80U)) {
            *index += 1U;
            return 0U;
        }
        *index += 3U;
        return (uint32_t)(((bytes[0] & 0x0FU) << 12) |
                          ((bytes[1] & 0x3FU) << 6) |
                          (bytes[2] & 0x3FU));
    }

    if((bytes[0] & 0xF8U) == 0xF0U) {
        if(((bytes[1] & 0xC0U) != 0x80U) ||
           ((bytes[2] & 0xC0U) != 0x80U) ||
           ((bytes[3] & 0xC0U) != 0x80U)) {
            *index += 1U;
            return 0U;
        }
        *index += 4U;
        return (uint32_t)(((bytes[0] & 0x07U) << 18) |
                          ((bytes[1] & 0x3FU) << 12) |
                          ((bytes[2] & 0x3FU) << 6) |
                          (bytes[3] & 0x3FU));
    }

    *index += 1U;
    return 0U;
}

static uint8_t custom_font_probe_text(const lv_font_t *font, const char *text, const char *tag)
{
    uint32_t index = 0U;
    uint32_t cp;
    lv_font_glyph_dsc_t dsc;
    const uint8_t *bmp;

    if((font == NULL) || (text == NULL) || (tag == NULL)) {
        return 0U;
    }

    while(text[index] != '\0') {
        cp = custom_utf8_next_codepoint(text, &index);
        if(cp == 0U) {
            printf("[FONT][SD24][%s] invalid utf8 sequence\r\n", tag);
            return 0U;
        }

        if(lv_font_get_glyph_dsc(font, &dsc, cp, 0U) == false) {
            printf("[FONT][SD24][%s] miss dsc U+%04lX\r\n", tag, (unsigned long)cp);
            return 0U;
        }

        bmp = lv_font_get_glyph_bitmap(font, cp);
        if(bmp == NULL) {
            printf("[FONT][SD24][%s] miss bmp U+%04lX\r\n", tag, (unsigned long)cp);
            return 0U;
        }

        if((cp > 0x7FU) && ((dsc.box_w == 0U) || (dsc.box_h == 0U))) {
            printf("[FONT][SD24][%s] bad box U+%04lX w=%u h=%u\r\n",
                   tag,
                   (unsigned long)cp,
                   (unsigned int)dsc.box_w,
                   (unsigned int)dsc.box_h);
            return 0U;
        }
    }

    return 1U;
}

static uint8_t custom_diag_sd24_weather_font(void)
{
    static const struct {
        const char *name;
        const char *text;
    } k_diag_texts[] = {
        {"QING", "\xE6\x99\xB4"},
        {"DUOYUN", "\xE5\xA4\x9A\xE4\xBA\x91"},
        {"SHAOYUN", "\xE5\xB0\x91\xE4\xBA\x91"},
        {"QINGJIAN_DUOYUN", "\xE6\x99\xB4\xE9\x97\xB4\xE5\xA4\x9A\xE4\xBA\x91"},
        {"YIN", "\xE9\x98\xB4"},
        {"ZHENYU", "\xE9\x98\xB5\xE9\x9B\xA8"},
        {"QIANG_ZHENYU", "\xE5\xBC\xBA\xE9\x98\xB5\xE9\x9B\xA8"},
        {"LEIZHENYU", "\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8"},
        {"QIANG_LEIZHENYU", "\xE5\xBC\xBA\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8"},
        {"XIAOYU", "\xE5\xB0\x8F\xE9\x9B\xA8"},
        {"ZHONGYU", "\xE4\xB8\xAD\xE9\x9B\xA8"},
        {"DAYU", "\xE5\xA4\xA7\xE9\x9B\xA8"},
        {"BAOYU", "\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"DA_BAOYU", "\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"TE_DA_BAOYU", "\xE7\x89\xB9\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"XIAO_DAO_ZHONGYU", "\xE5\xB0\x8F\xE5\x88\xB0\xE4\xB8\xAD\xE9\x9B\xA8"},
        {"ZHONG_DAO_DAYU", "\xE4\xB8\xAD\xE5\x88\xB0\xE5\xA4\xA7\xE9\x9B\xA8"},
        {"DA_DAO_BAOYU", "\xE5\xA4\xA7\xE5\x88\xB0\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"BAOYU_DAO_DA_BAOYU", "\xE6\x9A\xB4\xE9\x9B\xA8\xE5\x88\xB0\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"DA_BAOYU_DAO_TE_DA_BAOYU", "\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8\xE5\x88\xB0\xE7\x89\xB9\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8"},
        {"XIAOXUE", "\xE5\xB0\x8F\xE9\x9B\xAA"},
        {"ZHONGXUE", "\xE4\xB8\xAD\xE9\x9B\xAA"},
        {"DAXUE", "\xE5\xA4\xA7\xE9\x9B\xAA"},
        {"BAOXUE", "\xE6\x9A\xB4\xE9\x9B\xAA"},
        {"ZHENXUE", "\xE9\x98\xB5\xE9\x9B\xAA"},
        {"QIANG_ZHENXUE", "\xE5\xBC\xBA\xE9\x98\xB5\xE9\x9B\xAA"},
        {"YUXUE_TIANQI", "\xE9\x9B\xA8\xE9\x9B\xAA\xE5\xA4\xA9\xE6\xB0\x94"},
        {"YUJIAXUE", "\xE9\x9B\xA8\xE5\xA4\xB9\xE9\x9B\xAA"},
        {"ZHENYU_JIAXUE", "\xE9\x98\xB5\xE9\x9B\xA8\xE5\xA4\xB9\xE9\x9B\xAA"},
        {"BOWU", "\xE8\x96\x84\xE9\x9B\xBE"},
        {"WU", "\xE9\x9B\xBE"},
        {"NONGWU", "\xE6\xB5\x93\xE9\x9B\xBE"},
        {"QIANG_NONGWU", "\xE5\xBC\xBA\xE6\xB5\x93\xE9\x9B\xBE"},
        {"TEQIANG_NONGWU", "\xE7\x89\xB9\xE5\xBC\xBA\xE6\xB5\x93\xE9\x9B\xBE"},
        {"ZHONGDU_MAI", "\xE4\xB8\xAD\xE5\xBA\xA6\xE9\x9C\xBE"},
        {"ZHONG_MAI", "\xE9\x87\x8D\xE5\xBA\xA6\xE9\x9C\xBE"},
        {"YANZHONG_MAI", "\xE4\xB8\xA5\xE9\x87\x8D\xE9\x9C\xBE"},
        {"YANGSHA", "\xE6\x89\xAC\xE6\xB2\x99"},
        {"FUCHEN", "\xE6\xB5\xAE\xE5\xB0\x98"},
        {"SHACHEN_BAO", "\xE6\xB2\x99\xE5\xB0\x98\xE6\x9A\xB4"},
        {"QIANG_SHACHEN_BAO", "\xE5\xBC\xBA\xE6\xB2\x99\xE5\xB0\x98\xE6\x9A\xB4"},
        {"LENG", "\xE5\x86\xB7"},
        {"RE", "\xE7\x83\xAD"},
        {"WEIZHI", "\xE6\x9C\xAA\xE7\x9F\xA5"},
        {"QINGWU", "\xE8\xBD\xBB\xE9\x9B\xBE"},
        {"ZHUAN_DUOYUN", "\xE8\xBD\xAC\xE5\xA4\x9A\xE4\xBA\x91"},
        {"ZHUAN_ZHENYU", "\xE8\xBD\xAC\xE9\x98\xB5\xE9\x9B\xA8"},
        {"ZHUAN_LEIZHENYU", "\xE8\xBD\xAC\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8"},
        {"MAI", "\xE9\x9C\xBE"},
        {"LEIZHENYU_BINGBAO", "\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8\xE4\xBC\xB4\xE5\x86\xB0\xE9\x9B\xB9"}
    };
    uint32_t i;
    uint32_t pass_count = 0U;
    uint32_t fail_count = 0U;

    if(g_sd_font_24 == NULL) {
        return 0U;
    }

    for(i = 0U; i < (uint32_t)(sizeof(k_diag_texts) / sizeof(k_diag_texts[0])); i++) {
        if(custom_font_probe_text(g_sd_font_24, k_diag_texts[i].text, k_diag_texts[i].name) != 0U) {
            pass_count++;
        }
        else {
            fail_count++;
        }
    }

    return (fail_count == 0U) ? 1U : 0U;
}

static uint8_t custom_font_runtime_integrity_ok(const lv_font_t *font, const char *tag)
{
    const lv_font_fmt_txt_dsc_t *dsc;

    if((font == NULL) || (tag == NULL)) {
        return 0U;
    }

    dsc = (const lv_font_fmt_txt_dsc_t *)font->dsc;
    if(dsc == NULL) {
        printf("[FONT] %s invalid: dsc=NULL\r\n", tag);
        return 0U;
    }

    if(dsc->glyph_dsc == NULL) {
        printf("[FONT] %s invalid: glyph_dsc=NULL\r\n", tag);
        return 0U;
    }

    if(dsc->cmaps == NULL) {
        printf("[FONT] %s invalid: cmaps=NULL\r\n", tag);
        return 0U;
    }

    if(dsc->glyph_bitmap == NULL) {
        lv_mem_monitor_t mem_mon;
        lv_mem_monitor(&mem_mon);
        printf("[FONT] %s invalid: glyph_bitmap=NULL (possible LV_MEM shortage) free=%lu biggest=%lu used=%u frag=%u\r\n",
               tag,
               (unsigned long)mem_mon.free_size,
               (unsigned long)mem_mon.free_biggest_size,
               (unsigned int)mem_mon.used_pct,
               (unsigned int)mem_mon.frag_pct);
        return 0U;
    }

    return 1U;
}

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak uint8_t WeatherClock_SD_FontStorageInit(void)
#else
uint8_t __attribute__((weak)) WeatherClock_SD_FontStorageInit(void)
#endif
{
    return 0U;
}

static void custom_apply_sd_fonts(lv_ui *ui)
{
    lv_font_t *weather_main_font = NULL;
    lv_font_t *weather_day_font = NULL;

    if(ui == NULL) {
        return;
    }

#if WEATHERCLOCK_FORCE_WEATHER_USE_SD18
    if(g_sd_font_18 != NULL) {
        weather_main_font = g_sd_font_18;
        weather_day_font = g_sd_font_18;
    }
#else
    if(g_sd_font_18 != NULL) {
        weather_day_font = g_sd_font_18;
    }

    if(g_sd_font_24 != NULL) {
        weather_main_font = g_sd_font_24;
    }
#endif

    if((ui->screen_1_label_weather_main != NULL) && (weather_main_font != NULL)) {
        lv_obj_set_style_text_font(ui->screen_1_label_weather_main, weather_main_font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if((ui->screen_1_label_weather_day != NULL) && (weather_day_font != NULL)) {
        lv_obj_set_style_text_font(ui->screen_1_label_weather_day, weather_day_font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if(ui->screen_1_spangroup_3 != NULL) {
        lv_spangroup_refr_mode(ui->screen_1_spangroup_3);
    }
    if(ui->screen_1_spangroup_4 != NULL) {
        lv_spangroup_refr_mode(ui->screen_1_spangroup_4);
    }
    if(ui->screen_1_spangroup_12 != NULL) {
        lv_spangroup_refr_mode(ui->screen_1_spangroup_12);
    }

    if((g_sd_font_apply_error_log_done == 0U)
       && ((weather_main_font == NULL) || (weather_day_font == NULL))) {
        printf("[FONT] SD weather font apply failed: main=%p day=%p\r\n",
               (void *)weather_main_font,
               (void *)weather_day_font);
         printf("[FONT] Weather labels are SD-font only, check S:/font/FONT18.BIN and FONT24.BIN\r\n");
        g_sd_font_apply_error_log_done = 1U;
    }

#if WEATHERCLOCK_FORCE_WEATHER_USE_SD18
    if(g_sd_font_18 != NULL) {
        printf("[FONT] Weather labels forced to SD18 for diagnosis (24 disabled)\r\n");
    }
#endif

}

static void custom_try_load_sd_fonts(lv_ui *ui)
{
    if(g_sd_font_load_done != 0U) {
        custom_apply_sd_fonts(ui);
        return;
    }

    g_sd_font_load_done = 1U;

#if !WEATHERCLOCK_HAS_FATFS_HEADER
    printf("[FONT] FatFs missing: ff.h not found in project include paths\r\n");
    printf("[FONT] SD font loading is disabled, weather labels cannot use built-in fallback\r\n");
    return;
#endif

    if(WeatherClock_SD_FontStorageInit() == 0U) {
        printf("[FONT] SD/FATFS init hook failed, weather labels cannot use built-in fallback\r\n");
        return;
    }

#if LV_USE_FS_FATFS
    lv_fs_fatfs_init();
#endif

    {
        uint32_t size18 = custom_get_file_size("S:/font/FONT18.BIN");
        uint32_t size24 = custom_get_file_size("S:/font/FONT24.BIN");
        printf("[FONT] SD file exist: FONT18=%u FONT24=%u\r\n",
               (unsigned int)(size18 > 0U),
               (unsigned int)(size24 > 0U));
    }

    g_sd_font_18 = lv_font_load("S:/font/FONT18.BIN");
    if(g_sd_font_18 == NULL) {
        printf("[FONT] Load failed: S:/font/FONT18.BIN\r\n");
    }
    else if(custom_font_runtime_integrity_ok(g_sd_font_18, "SD18") == 0U) {
        g_sd_font_18 = NULL;
    }

    g_sd_font_24 = lv_font_load("S:/font/FONT24.BIN");
    if(g_sd_font_24 == NULL) {
        printf("[FONT] Load failed: S:/font/FONT24.BIN\r\n");
    }
    else if(custom_font_runtime_integrity_ok(g_sd_font_24, "SD24") == 0U) {
        g_sd_font_24 = NULL;
    }

    if(g_sd_font_18 != NULL) {
        g_sd_font_18_cjk_ok = custom_font_has_codepoints(g_sd_font_18,
                                                         k_weather_font_cp_18,
                                                         (uint16_t)(sizeof(k_weather_font_cp_18) / sizeof(k_weather_font_cp_18[0])),
                                                         "SD18");
    }

    if(g_sd_font_24 != NULL) {
        g_sd_font_24_cjk_ok = custom_font_has_codepoints(g_sd_font_24,
                                                         k_weather_font_cp_24,
                                                         (uint16_t)(sizeof(k_weather_font_cp_24) / sizeof(k_weather_font_cp_24[0])),
                                                         "SD24");
        if(custom_diag_sd24_weather_font() == 0U) {
            g_sd_font_24_cjk_ok = 0U;
        }
    }

    if((g_sd_font_18 != NULL) || (g_sd_font_24 != NULL)) {
        custom_apply_sd_fonts(ui);
    }
}

static int custom_calc_weekday_monday0(int year, int month, int day)
{
    int y;
    int m;
    int k;
    int j;
    int h;

    y = year;
    m = month;
    if(m < 3) {
        m += 12;
        y -= 1;
    }

    k = y % 100;
    j = y / 100;

    h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

    if(h == 0) return 5;
    if(h == 1) return 6;
    if(h == 2) return 0;
    if(h == 3) return 1;
    if(h == 4) return 2;
    if(h == 5) return 3;
    return 4;
}

/*
 * 星期字符串表（UTF-8）
 * 说明：
 * 1) 这里使用十六进制 UTF-8 字节序列，避免不同编辑器编码导致中文乱码。
 * 2) 下标约定：0~6 分别表示周一~周日。
 */
const char * week_days[7] = {
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xB8\x80", /* 星期一 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xBA\x8C", /* 星期二 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xB8\x89", /* 星期三 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE5\x9B\x9B", /* 星期四 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE4\xBA\x94", /* 星期五 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE5\x85\xAD", /* 星期六 */
    "\xE6\x98\x9F\xE6\x9C\x9F\xE6\x97\xA5"  /* 星期日 */
};

/*
 * 自定义初始化入口
 * 调用时机：UI 创建完成后，可在这里挂接自定义事件、动画或初始化业务状态。
 * 参数：
 * - ui: 由 Gui Guider 生成的全局 UI 结构体指针。
 * 当前实现：仅保留扩展点，不做额外逻辑。
 */

void custom_init(lv_ui *ui)
{
    int init_weekday_index;

    if(ui == NULL) {
        return;
    }

    if(lv_obj_is_valid(ui->screen_1_spangroup_12)) {
        lv_spangroup_set_mode(ui->screen_1_spangroup_12, LV_SPAN_MODE_BREAK);
        lv_spangroup_set_overflow(ui->screen_1_spangroup_12, LV_SPAN_OVERFLOW_CLIP);
        lv_obj_set_size(ui->screen_1_spangroup_12, 132, 40);
        lv_obj_set_style_pad_left(ui->screen_1_spangroup_12, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(ui->screen_1_spangroup_12, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_spangroup_refr_mode(ui->screen_1_spangroup_12);
    }

    if(ui->screen_1_spangroup_12_span != NULL) {
        lv_style_set_text_font(&ui->screen_1_spangroup_12_span->style, &lv_font_SourceHanSerifSC_Regular_30);
        lv_style_set_text_letter_space(&ui->screen_1_spangroup_12_span->style, 0);
    }

    custom_try_load_sd_fonts(ui);

    init_weekday_index = custom_calc_weekday_monday0(screen_1_date_year_init_value,
                                                      screen_1_date_month_init_value,
                                                      screen_1_date_day_init_value);
    custom_set_weekday_text(ui, init_weekday_index);
}

void custom_set_weekday_text(lv_ui *ui, int weekday_index)
{
    if(ui == NULL) {
        return;
    }

    if(weekday_index < 0 || weekday_index > 6) {
        weekday_index = 0;
    }

    if(!lv_obj_is_valid(ui->screen_1_spangroup_12)) {
        return;
    }

    if(ui->screen_1_spangroup_12_span != NULL) {
        lv_span_set_text(ui->screen_1_spangroup_12_span, "\xE6\x98\x9F\xE6\x9C\x9F");
        lv_style_set_text_font(&ui->screen_1_spangroup_12_span->style, &lv_font_SourceHanSerifSC_Regular_30);
        lv_style_set_text_letter_space(&ui->screen_1_spangroup_12_span->style, 0);
    }

    if(screen_1_weekday_day_span == NULL) {
        screen_1_weekday_day_span = lv_spangroup_new_span(ui->screen_1_spangroup_12);
        lv_style_set_text_color(&screen_1_weekday_day_span->style, lv_color_hex(0x000000));
        lv_style_set_text_decor(&screen_1_weekday_day_span->style, LV_TEXT_DECOR_NONE);
        lv_style_set_text_font(&screen_1_weekday_day_span->style, &lv_font_WeekdayCN_Subset_30);
    }

    lv_span_set_text(screen_1_weekday_day_span, week_day_suffix[weekday_index]);
    lv_spangroup_refr_mode(ui->screen_1_spangroup_12);
}

/*
 * 创建“数字时钟”对象（当前使用 lv_label 进行轻量实现）
 * 参数：
 * - parent: 父对象，时钟控件将挂在该对象下。
 * - input_time: 初始显示字符串，例如 "11:25 AM"。
 * 返回：
 * - 创建成功返回 label 对象指针。
 * 说明：
 * - 这里没有引入额外自定义类，直接复用 LVGL label，资源占用更小。
 */
lv_obj_t * lv_dclock_create(lv_obj_t * parent, const char * input_time)
{
    /* 创建用于显示时钟文本的标签 */
    lv_obj_t *label = lv_label_create(parent);

    /* 如果外部给了初始时间则使用外部值，否则使用默认值 */
    if(input_time != NULL) {
        lv_label_set_text(label, input_time);
    } else {
        lv_label_set_text(label, "12:00 AM");
    }

    /* 返回创建出的时钟对象 */
    return label;
}

/*
 * 按格式更新数字时钟文本
 * 参数：
 * - obj: 时钟对象（本实现中应为 label）。
 * - fmt: 格式化字符串，例如 "%d:%02d %s"。
 * - ...: 与 fmt 对应的可变参数。
 * 说明：
 * - 使用固定长度缓冲区（32 字节）控制栈占用。
 * - 若格式化结果超出 31 字符，vsnprintf 会自动截断并保证结尾 '\0'。
 */
void lv_dclock_set_text_fmt(lv_obj_t * obj, const char * fmt, ...)
{
    /* 本地格式化缓存：足够容纳常见时间字符串，如 "12:59 PM" */
    char buf[32];

    /* 启动可变参数读取 */
    va_list args;
    va_start(args, fmt);

    /* 格式化输出到缓冲区 */
    vsnprintf(buf, sizeof(buf), fmt, args);

    /* 结束可变参数读取 */
    va_end(args);

    /* 刷新到 LVGL 标签对象 */
    lv_label_set_text(obj, buf);
}

/*
 * 12 小时制时钟累加器（每调用一次，时间前进 1 秒）
 * 参数：
 * - hour/min/sec: 当前时分秒（原地修改）。
 * - meridiem: "AM" / "PM" 字符串缓冲区（至少 3 字节，含 '\0'）。
 * 逻辑：
 * 1) 秒进位到分；分进位到时。
 * 2) 时到 12 时切换 AM/PM。
 * 3) 时超过 12 回到 1。
 */
void clock_count_12(int *hour, int *min, int *sec, char *meridiem)
{
    /* 参数保护：任何指针为空都直接返回，避免非法访问 */
    if(hour == NULL || min == NULL || sec == NULL || meridiem == NULL) {
        return;
    }

    /* 秒 +1 */
    (*sec)++;

    /* 秒满 60，清零并给分钟进位 */
    if(*sec >= 60) {
        *sec = 0;
        (*min)++;
    }

    /* 分钟满 60，清零并给小时进位 */
    if(*min >= 60) {
        *min = 0;
        (*hour)++;

        /*
         * 小时到 12 时，切换 AM/PM
         * 示例：11:59:59 AM -> 12:00:00 PM
         */
        if(*hour == 12) {
            if(meridiem[0] == 'A' && meridiem[1] == 'M') {
                meridiem[0] = 'P';
                meridiem[1] = 'M';
            } else {
                meridiem[0] = 'A';
                meridiem[1] = 'M';
            }

            /* 明确补上字符串结束符，保证后续显示安全 */
            meridiem[2] = '\0';

        /* 超过 12 点后回到 1 点（12 小时制） */
        } else if(*hour > 12) {
            *hour = 1;
        }
    }
}

int is_leap_year(int year)
{
    if((year % 400) == 0) {
        return 1;
    }
    if((year % 100) == 0) {
        return 0;
    }
    return (year % 4) == 0;
}

int days_in_month(int year, int month)
{
    static const int days_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if(month < 1 || month > 12) {
        return 31;
    }

    if(month == 2 && is_leap_year(year)) {
        return 29;
    }

    return days_table[month - 1];
}

void date_count(int *year, int *month, int *day)
{
    int max_day;

    if(year == NULL || month == NULL || day == NULL) {
        return;
    }

    if(*month < 1 || *month > 12) {
        *month = 1;
    }

    if(*day < 1) {
        *day = 1;
    }

    max_day = days_in_month(*year, *month);
    if(*day > max_day) {
        *day = max_day;
    }

    (*day)++;
    max_day = days_in_month(*year, *month);
    if(*day > max_day) {
        *day = 1;
        (*month)++;

        if(*month > 12) {
            *month = 1;
            (*year)++;
        }
    }
}
