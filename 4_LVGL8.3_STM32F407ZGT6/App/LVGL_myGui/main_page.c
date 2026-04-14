#include "main_page.h"

#include <stdbool.h>

/*
 * 主页面（Weather Clock）
 * ------------------------------------------------------------
 * 设计说明：
 * 1) 本文件只使用 LVGL 对象 API 进行页面搭建，不直接操作 LCD 底层。
 * 2) 所有控件都挂载在当前活动屏幕 lv_scr_act() 上。
 * 3) 为避免元素重叠，中心天气图标区不再创建标题条，天气图形整体下移。
 */

#define CLR_BG          lv_color_hex(0x1a110f)
#define CLR_CYAN        lv_color_hex(0x05baf7)
#define CLR_WHITE       lv_color_hex(0xf2f7fb)
#define CLR_YELLOW      lv_color_hex(0xffcc16)
#define CLR_HUMIDITY    lv_color_hex(0x1db7ff)
#define CLR_PANEL_BG    lv_color_hex(0x1c120f)
#define CLR_LOW         lv_color_hex(0xffd400)

#define SEG_A (1U << 0)
#define SEG_B (1U << 1)
#define SEG_C (1U << 2)
#define SEG_D (1U << 3)
#define SEG_E (1U << 4)
#define SEG_F (1U << 5)
#define SEG_G (1U << 6)

/* 页面基础布局参数（480x320） */
#define FRAME_X          8
#define FRAME_Y          8
#define FRAME_W          464
#define FRAME_H          304

#define TOP_BAR_X        5
#define TOP_BAR_Y        5
#define TOP_BAR_W        454
#define TOP_BAR_H        34

#define PANEL_Y          44
#define PANEL_H          196

#define LEFT_PANEL_X     6
#define LEFT_PANEL_W     130

#define CENTER_PANEL_X   142
#define CENTER_PANEL_W   180

#define RIGHT_PANEL_X    328
#define RIGHT_PANEL_W    130

#define FORECAST_BAR_X   142
#define FORECAST_BAR_Y   218
#define FORECAST_BAR_W   180
#define FORECAST_BAR_H   22

#define BOTTOM_BAR_X     5
#define BOTTOM_BAR_Y     244
#define BOTTOM_BAR_W     454
#define BOTTOM_BAR_H     56

/* 将字符映射为 7 段数码管掩码 */
static uint8_t seg_mask_for_char(char ch)
{
    switch(ch) {
    case '0': return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
    case '1': return SEG_B | SEG_C;
    case '2': return SEG_A | SEG_B | SEG_G | SEG_E | SEG_D;
    case '3': return SEG_A | SEG_B | SEG_G | SEG_C | SEG_D;
    case '4': return SEG_F | SEG_G | SEG_B | SEG_C;
    case '5': return SEG_A | SEG_F | SEG_G | SEG_C | SEG_D;
    case '6': return SEG_A | SEG_F | SEG_G | SEG_C | SEG_D | SEG_E;
    case '7': return SEG_A | SEG_B | SEG_C;
    case '8': return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
    case '9': return SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
    case '-': return SEG_G;
    default: return 0;
    }
}

/* 创建圆角矩形基础图元（数码段/简单块） */
static lv_obj_t *create_rect(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t color, lv_opa_t opa)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, h / 2, 0);
    return obj;
}

/*
 * 创建通用面板。
 * title 为空字符串时，不创建顶部标题条（用于中间天气图区域，避免和图标重叠）。
 */
static lv_obj_t *create_panel(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, const char *title)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_style_bg_color(panel, CLR_PANEL_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_100, 0);
    lv_obj_set_style_border_color(panel, CLR_CYAN, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 8, 0);

    if(title != NULL && title[0] != '\0') {
        lv_obj_t *title_bar = lv_obj_create(panel);
        lv_obj_remove_style_all(title_bar);
        lv_obj_set_size(title_bar, w - 6, 22);
        lv_obj_set_pos(title_bar, 3, 3);
        lv_obj_set_style_bg_color(title_bar, CLR_CYAN, 0);
        lv_obj_set_style_bg_opa(title_bar, LV_OPA_90, 0);
        lv_obj_set_style_radius(title_bar, 4, 0);

        lv_obj_t *label = lv_label_create(title_bar);
        lv_label_set_text(label, title);
        lv_obj_set_style_text_color(label, lv_color_hex(0x0e1c2a), 0);
        lv_obj_center(label);
    }

    return panel;
}

/* 绘制单个 7 段数码字形 */
static void create_segment_digit(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t scale, lv_color_t on_color, lv_color_t off_color, lv_opa_t off_opa, char ch)
{
    lv_coord_t th = scale;
    lv_coord_t len = 4 * scale;

    uint8_t mask = seg_mask_for_char(ch);

    lv_color_t c_a = (mask & SEG_A) ? on_color : off_color;
    lv_color_t c_b = (mask & SEG_B) ? on_color : off_color;
    lv_color_t c_c = (mask & SEG_C) ? on_color : off_color;
    lv_color_t c_d = (mask & SEG_D) ? on_color : off_color;
    lv_color_t c_e = (mask & SEG_E) ? on_color : off_color;
    lv_color_t c_f = (mask & SEG_F) ? on_color : off_color;
    lv_color_t c_g = (mask & SEG_G) ? on_color : off_color;

    lv_opa_t o_a = (mask & SEG_A) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_b = (mask & SEG_B) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_c = (mask & SEG_C) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_d = (mask & SEG_D) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_e = (mask & SEG_E) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_f = (mask & SEG_F) ? LV_OPA_COVER : off_opa;
    lv_opa_t o_g = (mask & SEG_G) ? LV_OPA_COVER : off_opa;

    create_rect(parent, x + th, y + 0, len, th, c_a, o_a);
    create_rect(parent, x + len + th, y + th, th, len, c_b, o_b);
    create_rect(parent, x + len + th, y + len + 2 * th, th, len, c_c, o_c);
    create_rect(parent, x + th, y + 2 * len + 2 * th, len, th, c_d, o_d);
    create_rect(parent, x + 0, y + len + 2 * th, th, len, c_e, o_e);
    create_rect(parent, x + 0, y + th, th, len, c_f, o_f);
    create_rect(parent, x + th, y + len + th, len, th, c_g, o_g);
}

/* 连续绘制字符串形式的数码管数字 */
static void create_segment_number(lv_obj_t *parent, const char *num, lv_coord_t x, lv_coord_t y, lv_coord_t scale, lv_color_t on_color)
{
    lv_coord_t digit_w = 6 * scale;
    lv_coord_t spacing = scale + 1;
    lv_coord_t cursor = x;

    while(*num != '\0') {
        create_segment_digit(parent, cursor, y, scale, on_color, lv_color_hex(0x3f342f), LV_OPA_20, *num);
        cursor += digit_w + spacing;
        num++;
    }
}

/* 用多个圆角块组合云朵图形 */
static void create_cloud(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *base = lv_obj_create(parent);
    lv_obj_remove_style_all(base);
    lv_obj_set_size(base, w, h / 2);
    lv_obj_set_pos(base, x + w / 8, y + h / 2);
    lv_obj_set_style_bg_color(base, CLR_WHITE, 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(base, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *c1 = lv_obj_create(parent);
    lv_obj_remove_style_all(c1);
    lv_obj_set_size(c1, h / 2, h / 2);
    lv_obj_set_pos(c1, x, y + h / 3);
    lv_obj_set_style_bg_color(c1, CLR_WHITE, 0);
    lv_obj_set_style_bg_opa(c1, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(c1, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *c2 = lv_obj_create(parent);
    lv_obj_remove_style_all(c2);
    lv_obj_set_size(c2, h * 2 / 3, h * 2 / 3);
    lv_obj_set_pos(c2, x + w / 4, y);
    lv_obj_set_style_bg_color(c2, CLR_WHITE, 0);
    lv_obj_set_style_bg_opa(c2, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(c2, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *c3 = lv_obj_create(parent);
    lv_obj_remove_style_all(c3);
    lv_obj_set_size(c3, h / 2, h / 2);
    lv_obj_set_pos(c3, x + w / 2, y + h / 4);
    lv_obj_set_style_bg_color(c3, CLR_WHITE, 0);
    lv_obj_set_style_bg_opa(c3, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(c3, LV_RADIUS_CIRCLE, 0);
}

/* 创建主页面（静态示例数据） */
void main_page_create(void)
{
    /* 清理屏幕：若重复创建页面，先删除旧子对象避免层层叠加 */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, CLR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* 外边框 */
    lv_obj_t *frame = lv_obj_create(scr);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, FRAME_W, FRAME_H);
    lv_obj_set_pos(frame, FRAME_X, FRAME_Y);
    lv_obj_set_style_bg_color(frame, CLR_BG, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(frame, CLR_CYAN, 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_radius(frame, 10, 0);

    /* 顶栏：WiFi/时间/日期/闹钟 */
    lv_obj_t *top_bar = lv_obj_create(frame);
    lv_obj_remove_style_all(top_bar);
    lv_obj_set_size(top_bar, TOP_BAR_W, TOP_BAR_H);
    lv_obj_set_pos(top_bar, TOP_BAR_X, TOP_BAR_Y);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x231915), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 6, 0);

    lv_obj_t *wifi = lv_label_create(top_bar);
    lv_label_set_text(wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi, CLR_WHITE, 0);
    lv_obj_align(wifi, LV_ALIGN_LEFT_MID, 6, -1);

    lv_obj_t *wifi_txt = lv_label_create(top_bar);
    lv_label_set_text(wifi_txt, "WIFI");
    lv_obj_set_style_text_color(wifi_txt, CLR_WHITE, 0);
    lv_obj_align(wifi_txt, LV_ALIGN_LEFT_MID, 6, 10);

    lv_obj_t *time_txt = lv_label_create(top_bar);
    lv_label_set_text(time_txt, "12:00 pm   TUE");
    lv_obj_set_style_text_color(time_txt, CLR_WHITE, 0);
    lv_obj_align(time_txt, LV_ALIGN_CENTER, -20, 0);

    lv_obj_t *date_txt = lv_label_create(top_bar);
    lv_label_set_text(date_txt, "8*26");
    lv_obj_set_style_text_color(date_txt, CLR_WHITE, 0);
    lv_obj_align(date_txt, LV_ALIGN_RIGHT_MID, -40, -1);

    lv_obj_t *alarm_txt = lv_label_create(top_bar);
    lv_label_set_text(alarm_txt, "SNZ  " LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(alarm_txt, CLR_WHITE, 0);
    lv_obj_align(alarm_txt, LV_ALIGN_RIGHT_MID, -6, -1);

    /* 三栏内容区 */
    lv_obj_t *left = create_panel(frame, LEFT_PANEL_X, PANEL_Y, LEFT_PANEL_W, PANEL_H, "OUTDOOR");
    lv_obj_t *center = create_panel(frame, CENTER_PANEL_X, PANEL_Y, CENTER_PANEL_W, PANEL_H, "");
    lv_obj_t *right = create_panel(frame, RIGHT_PANEL_X, PANEL_Y, RIGHT_PANEL_W, PANEL_H, "INDOOR");

    /* 左栏：户外温湿度 */
    create_segment_number(left, "31", 14, 38, 8, CLR_YELLOW);
    lv_obj_t *l_temp = lv_label_create(left);
    lv_label_set_text(l_temp, "oC");
    lv_obj_set_style_text_color(l_temp, CLR_YELLOW, 0);
    lv_obj_set_pos(l_temp, 98, 52);

    lv_obj_t *l_th = lv_label_create(left);
    lv_label_set_text(l_th, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(l_th, CLR_YELLOW, 0);
    lv_obj_set_pos(l_th, 104, 72);

    create_segment_number(left, "73", 14, 118, 7, CLR_HUMIDITY);
    lv_obj_t *l_hu = lv_label_create(left);
    lv_label_set_text(l_hu, "%");
    lv_obj_set_style_text_color(l_hu, CLR_HUMIDITY, 0);
    lv_obj_set_pos(l_hu, 100, 130);

    lv_obj_t *l_drop = lv_label_create(left);
    lv_label_set_text(l_drop, LV_SYMBOL_TINT);
    lv_obj_set_style_text_color(l_drop, CLR_HUMIDITY, 0);
    lv_obj_set_pos(l_drop, 108, 148);

    /* 右栏：室内温湿度 */
    create_segment_number(right, "28", 14, 38, 8, CLR_YELLOW);
    lv_obj_t *r_temp = lv_label_create(right);
    lv_label_set_text(r_temp, "oC");
    lv_obj_set_style_text_color(r_temp, CLR_YELLOW, 0);
    lv_obj_set_pos(r_temp, 98, 52);

    lv_obj_t *r_th = lv_label_create(right);
    lv_label_set_text(r_th, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(r_th, CLR_YELLOW, 0);
    lv_obj_set_pos(r_th, 104, 72);

    create_segment_number(right, "63", 14, 118, 7, CLR_HUMIDITY);
    lv_obj_t *r_hu = lv_label_create(right);
    lv_label_set_text(r_hu, "%");
    lv_obj_set_style_text_color(r_hu, CLR_HUMIDITY, 0);
    lv_obj_set_pos(r_hu, 100, 130);

    lv_obj_t *r_drop = lv_label_create(right);
    lv_label_set_text(r_drop, LV_SYMBOL_TINT);
    lv_obj_set_style_text_color(r_drop, CLR_HUMIDITY, 0);
    lv_obj_set_pos(r_drop, 108, 148);

    /*
     * 中栏：天气图形
     * 重叠修正：
     * 1) 中栏取消了标题条（create_panel 传空字符串）
     * 2) 太阳与云整体下移，避免与顶部区域冲突
     */
    lv_obj_t *sun = lv_obj_create(center);
    lv_obj_remove_style_all(sun);
    lv_obj_set_size(sun, 52, 52);
    lv_obj_set_pos(sun, 108, 30);
    lv_obj_set_style_bg_color(sun, CLR_YELLOW, 0);
    lv_obj_set_style_bg_opa(sun, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);

    create_cloud(center, 26, 84, 78, 48);
    create_cloud(center, 92, 96, 68, 42);

    /* 中栏下沿预报标题条 */
    lv_obj_t *forecast_bar = lv_obj_create(frame);
    lv_obj_remove_style_all(forecast_bar);
    lv_obj_set_size(forecast_bar, FORECAST_BAR_W, FORECAST_BAR_H);
    lv_obj_set_pos(forecast_bar, FORECAST_BAR_X, FORECAST_BAR_Y);
    lv_obj_set_style_bg_color(forecast_bar, CLR_CYAN, 0);
    lv_obj_set_style_bg_opa(forecast_bar, LV_OPA_90, 0);
    lv_obj_set_style_radius(forecast_bar, 4, 0);

    lv_obj_t *forecast_txt = lv_label_create(forecast_bar);
    lv_label_set_text(forecast_txt, "FORECAST");
    lv_obj_set_style_text_color(forecast_txt, lv_color_hex(0x0e1c2a), 0);
    lv_obj_center(forecast_txt);

    /* 底部温度区 */
    lv_obj_t *bottom = lv_obj_create(frame);
    lv_obj_remove_style_all(bottom);
    lv_obj_set_size(bottom, BOTTOM_BAR_W, BOTTOM_BAR_H);
    lv_obj_set_pos(bottom, BOTTOM_BAR_X, BOTTOM_BAR_Y);
    lv_obj_set_style_bg_color(bottom, lv_color_hex(0x201612), 0);
    lv_obj_set_style_bg_opa(bottom, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bottom, 6, 0);

    lv_obj_t *low = lv_label_create(bottom);
    lv_label_set_text(low, "low");
    lv_obj_set_style_text_color(low, CLR_LOW, 0);
    lv_obj_set_pos(low, 8, 28);

    lv_obj_t *low_t = lv_label_create(bottom);
    lv_label_set_text(low_t, "27oC");
    lv_obj_set_style_text_color(low_t, CLR_LOW, 0);
    lv_obj_set_pos(low_t, 32, 20);

    lv_obj_t *high = lv_label_create(bottom);
    lv_label_set_text(high, "high");
    lv_obj_set_style_text_color(high, CLR_LOW, 0);
    lv_obj_set_pos(high, 104, 28);

    lv_obj_t *high_t = lv_label_create(bottom);
    lv_label_set_text(high_t, "33oC");
    lv_obj_set_style_text_color(high_t, CLR_LOW, 0);
    lv_obj_set_pos(high_t, 126, 20);
}
