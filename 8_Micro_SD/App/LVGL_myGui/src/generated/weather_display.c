#include "weather_display.h"

#include <stdio.h>
#include <string.h>

#include "lvgl.h"

static lv_ui *s_ui = NULL;
static lv_obj_t *s_weather_icon_main_obj = NULL;
static lv_obj_t *s_weather_icon_day_obj = NULL;
static int s_weather_icon_main_code_last = -9999;
static int s_weather_icon_day_code_last = -9999;
static char s_weather_icon_main_path[64] = {0};
static char s_weather_icon_day_path[64] = {0};

static const char *SelectNonEmptyText(const char *first, const char *second, const char *third)
{
    if((first != NULL) && (first[0] != '\0'))
    {
        return first;
    }
    if((second != NULL) && (second[0] != '\0'))
    {
        return second;
    }
    if((third != NULL) && (third[0] != '\0'))
    {
        return third;
    }
    return "--";
}

static uint8_t WeatherIconFileExists(const char *path)
{
    lv_fs_file_t file;

    if(path == NULL)
    {
        return 0U;
    }

    if(lv_fs_open(&file, path, LV_FS_MODE_RD) != LV_FS_RES_OK)
    {
        return 0U;
    }

    (void)lv_fs_close(&file);
    return 1U;
}

static int NormalizeWeatherIconCode(int code)
{
    switch(code)
    {
        case 150:
            return 100;
        case 151:
            return 101;
        case 152:
            return 102;
        case 153:
            return 103;
        default:
            return code;
    }
}

static void BuildWeatherIconPath(int code, char *out_path, size_t out_size)
{
    int icon_code;

    if((out_path == NULL) || (out_size == 0U))
    {
        return;
    }

    icon_code = NormalizeWeatherIconCode(code);
    (void)snprintf(out_path, out_size, "S:/icon/weather/%d.bin", icon_code);
}

static void EnsureWeatherIconObjects(void)
{
    if((s_ui == NULL) || (s_ui->screen_1_cont_1 == NULL))
    {
        return;
    }

    if((s_weather_icon_main_obj != NULL) && (lv_obj_is_valid(s_weather_icon_main_obj) == false))
    {
        s_weather_icon_main_obj = NULL;
        s_weather_icon_main_code_last = -9999;
        s_weather_icon_main_path[0] = '\0';
    }

    if((s_weather_icon_day_obj != NULL) && (lv_obj_is_valid(s_weather_icon_day_obj) == false))
    {
        s_weather_icon_day_obj = NULL;
        s_weather_icon_day_code_last = -9999;
        s_weather_icon_day_path[0] = '\0';
    }

    if((s_ui->screen_1_img_11 != NULL) && (lv_obj_is_valid(s_ui->screen_1_img_11) != false))
    {
        lv_obj_add_flag(s_ui->screen_1_img_11, LV_OBJ_FLAG_HIDDEN);
    }

    if(s_weather_icon_main_obj == NULL)
    {
        s_weather_icon_main_obj = lv_img_create(s_ui->screen_1_cont_1);
        lv_obj_add_flag(s_weather_icon_main_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_img_set_pivot(s_weather_icon_main_obj, 50, 50);
        lv_img_set_angle(s_weather_icon_main_obj, 0);
        lv_obj_set_pos(s_weather_icon_main_obj, 149, 61);
        lv_obj_set_size(s_weather_icon_main_obj, 88, 67);
        lv_obj_set_style_img_recolor_opa(s_weather_icon_main_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_img_opa(s_weather_icon_main_obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_weather_icon_main_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(s_weather_icon_main_obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if(s_weather_icon_day_obj == NULL)
    {
        s_weather_icon_day_obj = lv_img_create(s_ui->screen_1_cont_1);
        lv_obj_add_flag(s_weather_icon_day_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_img_set_pivot(s_weather_icon_day_obj, 50, 50);
        lv_img_set_angle(s_weather_icon_day_obj, 0);
        lv_obj_set_pos(s_weather_icon_day_obj, 246, 61);
        lv_obj_set_size(s_weather_icon_day_obj, 88, 67);
        lv_obj_set_style_img_recolor_opa(s_weather_icon_day_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_img_opa(s_weather_icon_day_obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(s_weather_icon_day_obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(s_weather_icon_day_obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void SetWeatherIconByCode(lv_obj_t *img_obj,
                                 int code,
                                 int *last_code,
                                 char *path_buf,
                                 size_t path_buf_size,
                                 const char *tag)
{
    char target_path[64];
    uint8_t file_ok;

    if((img_obj == NULL) || (last_code == NULL) || (path_buf == NULL) || (tag == NULL))
    {
        return;
    }

    if(code < 0)
    {
        lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);
        *last_code = code;
        return;
    }

    BuildWeatherIconPath(code, target_path, sizeof(target_path));
    file_ok = WeatherIconFileExists(target_path);

    if(file_ok != 0U)
    {
        (void)snprintf(path_buf, path_buf_size, "%s", target_path);
        lv_img_set_src(img_obj, path_buf);
        lv_obj_clear_flag(img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(img_obj);
        printf("[UI][ICON] %s code=%d file_ok=1 src='%s'\r\n",
               tag,
               code,
               path_buf);
    }
    else
    {
        lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);
        printf("[UI][ICON] %s code=%d file_ok=0 src='%s'\r\n",
               tag,
               code,
               target_path);
    }

    *last_code = code;
}

static int ResolveMainWeatherCode(const WeatherInfo *weather)
{
    if(weather == NULL)
    {
        return -1;
    }

    if(weather->hasWeatherCode != 0U)
    {
        return weather->weatherCode;
    }
    if(weather->hasTodayCodeDay != 0U)
    {
        return weather->todayCodeDay;
    }
    if(weather->hasTodayCodeNight != 0U)
    {
        return weather->todayCodeNight;
    }

    return -1;
}

static int ResolveDayWeatherCode(const WeatherInfo *weather)
{
    if(weather == NULL)
    {
        return -1;
    }

    if(weather->hasTodayCodeDay != 0U)
    {
        return weather->todayCodeDay;
    }
    if(weather->hasTodayCodeNight != 0U)
    {
        return weather->todayCodeNight;
    }
    if(weather->hasWeatherCode != 0U)
    {
        return weather->weatherCode;
    }

    return -1;
}

static void UpdateWeatherIcons(const WeatherInfo *weather)
{
    int main_code;
    int day_code;

    EnsureWeatherIconObjects();

    if((s_weather_icon_main_obj == NULL) || (s_weather_icon_day_obj == NULL))
    {
        return;
    }

    main_code = ResolveMainWeatherCode(weather);
    day_code = ResolveDayWeatherCode(weather);

    SetWeatherIconByCode(s_weather_icon_main_obj,
                         main_code,
                         &s_weather_icon_main_code_last,
                         s_weather_icon_main_path,
                         sizeof(s_weather_icon_main_path),
                         "main");

    SetWeatherIconByCode(s_weather_icon_day_obj,
                         day_code,
                         &s_weather_icon_day_code_last,
                         s_weather_icon_day_path,
                         sizeof(s_weather_icon_day_path),
                         "day");
}

static void LogLabelGlyphProbe(const char *tag, lv_obj_t *label, const char *text)
{
    (void)tag;
    (void)label;
    (void)text;
}

static const char *WeatherCodeToText(int code)
{
    static const char *TXT_QING = "\xE6\x99\xB4";
    static const char *TXT_DUOYUN = "\xE5\xA4\x9A\xE4\xBA\x91";
    static const char *TXT_SHAOYUN = "\xE5\xB0\x91\xE4\xBA\x91";
    static const char *TXT_QINGJIAN_DUOYUN = "\xE6\x99\xB4\xE9\x97\xB4\xE5\xA4\x9A\xE4\xBA\x91";
    static const char *TXT_YIN = "\xE9\x98\xB4";
    static const char *TXT_ZHENYU = "\xE9\x98\xB5\xE9\x9B\xA8";
    static const char *TXT_QIANG_ZHENYU = "\xE5\xBC\xBA\xE9\x98\xB5\xE9\x9B\xA8";
    static const char *TXT_LEIZHENYU = "\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8";
    static const char *TXT_QIANG_LEIZHENYU = "\xE5\xBC\xBA\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8";
    static const char *TXT_LEIZHENYU_BINGBAO = "\xE9\x9B\xB7\xE9\x98\xB5\xE9\x9B\xA8\xE4\xBC\xB4\xE5\x86\xB0\xE9\x9B\xB9";
    static const char *TXT_XIAOYU = "\xE5\xB0\x8F\xE9\x9B\xA8";
    static const char *TXT_ZHONGYU = "\xE4\xB8\xAD\xE9\x9B\xA8";
    static const char *TXT_DAYU = "\xE5\xA4\xA7\xE9\x9B\xA8";
    static const char *TXT_JIDUAN_JIANGYU = "\xE6\x9E\x81\xE7\xAB\xAF\xE9\x99\x8D\xE9\x9B\xA8";
    static const char *TXT_MAOMAOYU = "\xE6\xAF\x9B\xE6\xAF\x9B\xE9\x9B\xA8";
    static const char *TXT_BAOYU = "\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_DA_BAOYU = "\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_TE_DA_BAOYU = "\xE7\x89\xB9\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_DONGYU = "\xE5\x86\xBB\xE9\x9B\xA8";
    static const char *TXT_XIAO_DAO_ZHONGYU = "\xE5\xB0\x8F\xE5\x88\xB0\xE4\xB8\xAD\xE9\x9B\xA8";
    static const char *TXT_ZHONG_DAO_DAYU = "\xE4\xB8\xAD\xE5\x88\xB0\xE5\xA4\xA7\xE9\x9B\xA8";
    static const char *TXT_DA_DAO_BAOYU = "\xE5\xA4\xA7\xE5\x88\xB0\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_BAOYU_DAO_DA_BAOYU = "\xE6\x9A\xB4\xE9\x9B\xA8\xE5\x88\xB0\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_DA_BAOYU_DAO_TE_DA_BAOYU = "\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8\xE5\x88\xB0\xE7\x89\xB9\xE5\xA4\xA7\xE6\x9A\xB4\xE9\x9B\xA8";
    static const char *TXT_ZHENXUE = "\xE9\x98\xB5\xE9\x9B\xAA";
    static const char *TXT_QIANG_ZHENXUE = "\xE5\xBC\xBA\xE9\x98\xB5\xE9\x9B\xAA";
    static const char *TXT_YUXUE_TIANQI = "\xE9\x9B\xA8\xE9\x9B\xAA\xE5\xA4\xA9\xE6\xB0\x94";
    static const char *TXT_XIAOXUE = "\xE5\xB0\x8F\xE9\x9B\xAA";
    static const char *TXT_ZHONGXUE = "\xE4\xB8\xAD\xE9\x9B\xAA";
    static const char *TXT_DAXUE = "\xE5\xA4\xA7\xE9\x9B\xAA";
    static const char *TXT_BAOXUE = "\xE6\x9A\xB4\xE9\x9B\xAA";
    static const char *TXT_YUJIAXUE = "\xE9\x9B\xA8\xE5\xA4\xB9\xE9\x9B\xAA";
    static const char *TXT_ZHENYU_JIAXUE = "\xE9\x98\xB5\xE9\x9B\xA8\xE5\xA4\xB9\xE9\x9B\xAA";
    static const char *TXT_BOWU = "\xE8\x96\x84\xE9\x9B\xBE";
    static const char *TXT_WU = "\xE9\x9B\xBE";
    static const char *TXT_MAI = "\xE9\x9C\xBE";
    static const char *TXT_YANGSHA = "\xE6\x89\xAC\xE6\xB2\x99";
    static const char *TXT_FUCHEN = "\xE6\xB5\xAE\xE5\xB0\x98";
    static const char *TXT_SHACHEN_BAO = "\xE6\xB2\x99\xE5\xB0\x98\xE6\x9A\xB4";
    static const char *TXT_QIANG_SHACHEN_BAO = "\xE5\xBC\xBA\xE6\xB2\x99\xE5\xB0\x98\xE6\x9A\xB4";
    static const char *TXT_NONGWU = "\xE6\xB5\x93\xE9\x9B\xBE";
    static const char *TXT_QIANG_NONGWU = "\xE5\xBC\xBA\xE6\xB5\x93\xE9\x9B\xBE";
    static const char *TXT_ZHONGDU_MAI = "\xE4\xB8\xAD\xE5\xBA\xA6\xE9\x9C\xBE";
    static const char *TXT_ZHONG_MAI = "\xE9\x87\x8D\xE5\xBA\xA6\xE9\x9C\xBE";
    static const char *TXT_YANZHONG_MAI = "\xE4\xB8\xA5\xE9\x87\x8D\xE9\x9C\xBE";
    static const char *TXT_DAWU = "\xE5\xA4\xA7\xE9\x9B\xBE";
    static const char *TXT_TEQIANG_NONGWU = "\xE7\x89\xB9\xE5\xBC\xBA\xE6\xB5\x93\xE9\x9B\xBE";
    static const char *TXT_RE = "\xE7\x83\xAD";
    static const char *TXT_LENG = "\xE5\x86\xB7";
    static const char *TXT_WEIZHI = "\xE6\x9C\xAA\xE7\x9F\xA5";

    code = NormalizeWeatherIconCode(code);

    switch(code)
    {
        case 100:
        case 150:
            return TXT_QING;
        case 101:
        case 151:
            return TXT_DUOYUN;
        case 102:
        case 152:
            return TXT_SHAOYUN;
        case 103:
        case 153:
            return TXT_QINGJIAN_DUOYUN;
        case 104:
            return TXT_YIN;
        case 300:
            return TXT_ZHENYU;
        case 301:
            return TXT_QIANG_ZHENYU;
        case 302:
            return TXT_LEIZHENYU;
        case 303:
            return TXT_QIANG_LEIZHENYU;
        case 304:
            return TXT_LEIZHENYU_BINGBAO;
        case 305:
            return TXT_XIAOYU;
        case 306:
            return TXT_ZHONGYU;
        case 307:
            return TXT_DAYU;
        case 308:
            return TXT_JIDUAN_JIANGYU;
        case 309:
            return TXT_MAOMAOYU;
        case 310:
            return TXT_BAOYU;
        case 311:
            return TXT_DA_BAOYU;
        case 312:
            return TXT_TE_DA_BAOYU;
        case 313:
            return TXT_DONGYU;
        case 314:
            return TXT_XIAO_DAO_ZHONGYU;
        case 315:
            return TXT_ZHONG_DAO_DAYU;
        case 316:
            return TXT_DA_DAO_BAOYU;
        case 317:
            return TXT_BAOYU_DAO_DA_BAOYU;
        case 318:
            return TXT_DA_BAOYU_DAO_TE_DA_BAOYU;
        case 350:
        case 407:
            return TXT_ZHENXUE;
        case 351:
            return TXT_QIANG_ZHENXUE;
        case 399:
        case 405:
            return TXT_YUXUE_TIANQI;
        case 400:
            return TXT_XIAOXUE;
        case 401:
            return TXT_ZHONGXUE;
        case 402:
            return TXT_DAXUE;
        case 403:
            return TXT_BAOXUE;
        case 404:
            return TXT_YUJIAXUE;
        case 406:
            return TXT_ZHENYU_JIAXUE;
        case 500:
            return TXT_BOWU;
        case 501:
            return TXT_WU;
        case 502:
            return TXT_MAI;
        case 503:
            return TXT_YANGSHA;
        case 504:
            return TXT_FUCHEN;
        case 507:
            return TXT_SHACHEN_BAO;
        case 508:
            return TXT_QIANG_SHACHEN_BAO;
        case 509:
            return TXT_NONGWU;
        case 510:
            return TXT_QIANG_NONGWU;
        case 511:
            return TXT_ZHONGDU_MAI;
        case 512:
            return TXT_ZHONG_MAI;
        case 513:
            return TXT_YANZHONG_MAI;
        case 514:
            return TXT_DAWU;
        case 515:
            return TXT_TEQIANG_NONGWU;
        case 900:
            return TXT_RE;
        case 901:
            return TXT_LENG;
        case 999:
            return TXT_WEIZHI;
        default:
            return NULL;
    }
}

static void UpdateWeatherTextLabels(const WeatherInfo *weather)
{
    const char *weather_text = "--";
    const char *today_day_text = "--";
    const char *today_night_text = "--";
    const char *main_text = "--";
    const char *day_text = "--";
    const char *main_by_code = NULL;
    const char *day_by_code = NULL;

    if((s_ui == NULL) || (weather == NULL))
    {
        return;
    }

    if(weather->textUtf8Valid != 0U)
    {
        weather_text = SelectNonEmptyText(weather->text, NULL, NULL);
    }
    if(weather->todayTextDayUtf8Valid != 0U)
    {
        today_day_text = SelectNonEmptyText(weather->todayTextDay, NULL, NULL);
    }
    if(weather->todayTextNightUtf8Valid != 0U)
    {
        today_night_text = SelectNonEmptyText(weather->todayTextNight, NULL, NULL);
    }
    main_text = SelectNonEmptyText(weather_text, today_day_text, today_night_text);
    day_text = SelectNonEmptyText(today_day_text, today_night_text, weather_text);

    if(weather->hasWeatherCode != 0U)
    {
        main_by_code = WeatherCodeToText(weather->weatherCode);
    }
    else if(weather->hasTodayCodeDay != 0U)
    {
        main_by_code = WeatherCodeToText(weather->todayCodeDay);
    }
    else if(weather->hasTodayCodeNight != 0U)
    {
        main_by_code = WeatherCodeToText(weather->todayCodeNight);
    }

    if(weather->hasTodayCodeDay != 0U)
    {
        day_by_code = WeatherCodeToText(weather->todayCodeDay);
    }
    else if(weather->hasTodayCodeNight != 0U)
    {
        day_by_code = WeatherCodeToText(weather->todayCodeNight);
    }
    else if(weather->hasWeatherCode != 0U)
    {
        day_by_code = WeatherCodeToText(weather->weatherCode);
    }

    if(main_by_code != NULL)
    {
        main_text = main_by_code;
    }
    if(day_by_code != NULL)
    {
        day_text = day_by_code;
    }

    if(s_ui->screen_1_label_weather_main != NULL)
    {
        const lv_font_t *main_font = lv_obj_get_style_text_font(s_ui->screen_1_label_weather_main,
                                                                 LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_coord_t main_h = 40;

        if(main_font != NULL)
        {
            lv_coord_t font_h = (lv_coord_t)main_font->line_height + 6;
            if(font_h > main_h)
            {
                main_h = font_h;
            }
            if(main_h > 72)
            {
                main_h = 72;
            }
        }

        lv_obj_clear_flag(s_ui->screen_1_label_weather_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(s_ui->screen_1_label_weather_main, 108, main_h);
        lv_label_set_long_mode(s_ui->screen_1_label_weather_main, LV_LABEL_LONG_CLIP);
        lv_obj_move_foreground(s_ui->screen_1_label_weather_main);
        lv_label_set_text(s_ui->screen_1_label_weather_main, main_text);
        lv_obj_invalidate(s_ui->screen_1_label_weather_main);
        LogLabelGlyphProbe("main", s_ui->screen_1_label_weather_main, main_text);
    }

    if(s_ui->screen_1_label_weather_day != NULL)
    {
        const lv_font_t *day_font = lv_obj_get_style_text_font(s_ui->screen_1_label_weather_day,
                                                                LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_coord_t day_h = 28;

        if(day_font != NULL)
        {
            lv_coord_t font_h = (lv_coord_t)day_font->line_height + 6;
            if(font_h > day_h)
            {
                day_h = font_h;
            }
            if(day_h > 48)
            {
                day_h = 48;
            }
        }

        lv_obj_clear_flag(s_ui->screen_1_label_weather_day, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(s_ui->screen_1_label_weather_day, 76, day_h);
        lv_label_set_long_mode(s_ui->screen_1_label_weather_day, LV_LABEL_LONG_CLIP);
        lv_obj_move_foreground(s_ui->screen_1_label_weather_day);
        lv_label_set_text(s_ui->screen_1_label_weather_day, day_text);
        lv_obj_invalidate(s_ui->screen_1_label_weather_day);
        LogLabelGlyphProbe("day", s_ui->screen_1_label_weather_day, day_text);
    }

    UpdateWeatherIcons(weather);
}

void WeatherDisplay_Init(lv_ui *ui)
{
    s_ui = ui;
    s_weather_icon_main_obj = NULL;
    s_weather_icon_day_obj = NULL;
    s_weather_icon_main_code_last = -9999;
    s_weather_icon_day_code_last = -9999;
    s_weather_icon_main_path[0] = '\0';
    s_weather_icon_day_path[0] = '\0';
}

void WeatherDisplay_Update(const WeatherInfo *weather)
{
    if(s_ui == NULL)
    {
        return;
    }

    UpdateWeatherTextLabels(weather);
}
