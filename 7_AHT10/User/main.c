#include "LCD_delay.h"
#include "sys.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "LCD_test.h"
#include "usart.h"
#include "led.h"

#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include "AHT10.h"

#include <stdio.h>
#include <string.h>

#include "lvgl.h"                // 它为整个LVGL提供了更完整的头文件引用
#include "lv_port_disp.h"        // LVGL的显示支持
#include "lv_port_indev.h"       // LVGL的触屏支持

#include "FreeRTOS.h"
#include "task.h"

lv_ui guider_ui;
static Esp32ProtocolContext g_esp32Ctx;
static const TickType_t WIFI_BLINK_INTERVAL = pdMS_TO_TICKS(500);
static const TickType_t WIFI_DISCONNECT_BLINK_DELAY = pdMS_TO_TICKS(30000);
static const TickType_t WIFI_DISCONNECT_PROBE_INTERVAL = pdMS_TO_TICKS(30000);
static uint8_t g_wifi_connected = 0U;
static uint8_t g_wifi_status_known = 0U;
static uint8_t g_wifi_icon_visible = 1U;
static TickType_t g_wifi_blink_tick = 0;
static TickType_t g_wifi_disconnect_tick = 0;
static TickType_t g_wifi_probe_tick = 0;

extern int screen_1_digital_clock_1_hour_value;
extern int screen_1_digital_clock_1_min_value;
extern int screen_1_digital_clock_1_sec_value;
extern char screen_1_digital_clock_1_meridiem[];

static int CalcWeekdayMonday0(int year, int month, int day)
{
    int y = year;
    int m = month;
    int k;
    int j;
    int h;

    if(m < 3)
    {
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

static void UpdateOutdoorTempHumi(int temp, int humidity, int todayMin, int todayMax)
{
    char temp_text[8];
    char humi_text[8];
    char low_text[8];
    char high_text[8];

    snprintf(temp_text, sizeof(temp_text), "%d", temp);
    snprintf(humi_text, sizeof(humi_text), "%d", humidity);
    snprintf(low_text, sizeof(low_text), "%d", todayMin);
    snprintf(high_text, sizeof(high_text), "%d", todayMax);

    if(guider_ui.screen_1_spangroup_10_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_10_span, temp_text);
    }

    if(guider_ui.screen_1_spangroup_11_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_11_span, humi_text);
    }

    if(guider_ui.screen_1_spangroup_13_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_13_span, low_text);
    }

    if(guider_ui.screen_1_spangroup_14_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_14_span, high_text);
    }

    if(guider_ui.screen_1_spangroup_10 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_10);
    }

    if(guider_ui.screen_1_spangroup_11 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_11);
    }

    if(guider_ui.screen_1_spangroup_13 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_13);
    }

    if(guider_ui.screen_1_spangroup_14 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_14);
    }
}

static void UpdateWeatherTextLabels(const char *weatherText, const char *todayTextDay)
{
    if(guider_ui.screen_1_label_weather_main != NULL)
    {
        if((weatherText != NULL) && (weatherText[0] != '\0'))
        {
            lv_label_set_text(guider_ui.screen_1_label_weather_main, weatherText);
        }
        else
        {
            lv_label_set_text(guider_ui.screen_1_label_weather_main, "--");
        }
    }

    if(guider_ui.screen_1_label_weather_day != NULL)
    {
        if((todayTextDay != NULL) && (todayTextDay[0] != '\0'))
        {
            lv_label_set_text(guider_ui.screen_1_label_weather_day, todayTextDay);
        }
        else
        {
            lv_label_set_text(guider_ui.screen_1_label_weather_day, "--");
        }
    }
}

static void UpdateOutdoorTempHumiUnavailable(void)
{
    if(guider_ui.screen_1_spangroup_10_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_10_span, "--");
    }

    if(guider_ui.screen_1_spangroup_11_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_11_span, "--");
    }

    if(guider_ui.screen_1_spangroup_10 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_10);
    }

    if(guider_ui.screen_1_spangroup_11 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_11);
    }

    UpdateWeatherTextLabels("--", "--");
}

static void SetWifiIconVisible(uint8_t visible)
{
    if(guider_ui.screen_1_img_2 == NULL)
    {
        return;
    }

    if(visible != 0U)
    {
        lv_obj_clear_flag(guider_ui.screen_1_img_2, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(guider_ui.screen_1_img_2, LV_OBJ_FLAG_HIDDEN);
    }
}

static void UpdateWifiIndicatorUi(TickType_t now_tick)
{
    if(g_wifi_status_known == 0U)
    {
        return;
    }

    if(g_wifi_connected != 0U)
    {
        if(g_wifi_icon_visible == 0U)
        {
            g_wifi_icon_visible = 1U;
            SetWifiIconVisible(1U);
        }
        return;
    }

    if(now_tick - g_wifi_disconnect_tick < WIFI_DISCONNECT_BLINK_DELAY)
    {
        if(g_wifi_icon_visible == 0U)
        {
            g_wifi_icon_visible = 1U;
            SetWifiIconVisible(1U);
        }
        return;
    }

    if(now_tick - g_wifi_blink_tick >= WIFI_BLINK_INTERVAL)
    {
        g_wifi_blink_tick = now_tick;
        g_wifi_icon_visible = (g_wifi_icon_visible == 0U) ? 1U : 0U;
        SetWifiIconVisible(g_wifi_icon_visible);
    }
}

static void UpdateUiClockByTimeSync(const TimeSyncInfo *timeSync)
{
    int hour24;
    int hour12;
    int weekdayIndex;

    if((timeSync == NULL) || (timeSync->valid == 0U))
    {
        return;
    }

    hour24 = timeSync->hour;
    if(hour24 < 0)
    {
        hour24 = 0;
    }
    if(hour24 > 23)
    {
        hour24 = 23;
    }

    if(hour24 == 0)
    {
        hour12 = 12;
        screen_1_digital_clock_1_meridiem[0] = 'A';
        screen_1_digital_clock_1_meridiem[1] = 'M';
        screen_1_digital_clock_1_meridiem[2] = '\0';
    }
    else if(hour24 < 12)
    {
        hour12 = hour24;
        screen_1_digital_clock_1_meridiem[0] = 'A';
        screen_1_digital_clock_1_meridiem[1] = 'M';
        screen_1_digital_clock_1_meridiem[2] = '\0';
    }
    else if(hour24 == 12)
    {
        hour12 = 12;
        screen_1_digital_clock_1_meridiem[0] = 'P';
        screen_1_digital_clock_1_meridiem[1] = 'M';
        screen_1_digital_clock_1_meridiem[2] = '\0';
    }
    else
    {
        hour12 = hour24 - 12;
        screen_1_digital_clock_1_meridiem[0] = 'P';
        screen_1_digital_clock_1_meridiem[1] = 'M';
        screen_1_digital_clock_1_meridiem[2] = '\0';
    }

    screen_1_digital_clock_1_hour_value = hour12;
    screen_1_digital_clock_1_min_value = timeSync->min;
    screen_1_digital_clock_1_sec_value = timeSync->sec;

    if(guider_ui.screen_1_digital_clock_1 != NULL)
    {
        lv_dclock_set_text_fmt(guider_ui.screen_1_digital_clock_1,
                               "%d:%02d %s",
                               screen_1_digital_clock_1_hour_value,
                               screen_1_digital_clock_1_min_value,
                               screen_1_digital_clock_1_meridiem);
    }

    if(guider_ui.screen_1_datetext_1 != NULL)
    {
        lv_label_set_text_fmt(guider_ui.screen_1_datetext_1,
                              "%d/%02d/%02d",
                              timeSync->year,
                              timeSync->month,
                              timeSync->day);
    }

    weekdayIndex = CalcWeekdayMonday0(timeSync->year, timeSync->month, timeSync->day);
    custom_set_weekday_text(&guider_ui, weekdayIndex);
}

static void ProcessEsp32Protocol(void)
{
    char line[ESP32_UART_LINE_MAX_LEN];

    while(Esp32Uart_GetLine(line, (uint16_t)sizeof(line)) != 0U)
    {
        Esp32ParseEvent evt = Esp32Protocol_ParseLine(&g_esp32Ctx, line);

        if(evt == ESP32_EVT_PONG)
        {
            printf("[ESP32] PONG\r\n");
        }
        else if(evt == ESP32_EVT_TIME_SYNC)
        {
            UpdateUiClockByTimeSync(&g_esp32Ctx.timeSync);
            printf("[ESP32] TIME_SYNC %04d-%02d-%02d %02d:%02d:%02d\r\n",
                   g_esp32Ctx.timeSync.year,
                   g_esp32Ctx.timeSync.month,
                   g_esp32Ctx.timeSync.day,
                   g_esp32Ctx.timeSync.hour,
                   g_esp32Ctx.timeSync.min,
                   g_esp32Ctx.timeSync.sec);
            g_esp32Ctx.timeSyncReady = 0U;
        }
        else if(evt == ESP32_EVT_WEATHER_READY)
        {
            if(g_wifi_connected != 0U)
            {
                UpdateOutdoorTempHumi(g_esp32Ctx.weather.temp,
                                      g_esp32Ctx.weather.humidity,
                                      g_esp32Ctx.weather.todayTempMin,
                                      g_esp32Ctx.weather.todayTempMax);
                UpdateWeatherTextLabels(g_esp32Ctx.weather.text, g_esp32Ctx.weather.todayTextDay);
            }
            printf("[ESP32] WEATHER temp=%d humi=%d press=%d todayMax=%d todayMin=%d\r\n",
                   g_esp32Ctx.weather.temp,
                   g_esp32Ctx.weather.humidity,
                   g_esp32Ctx.weather.pressure,
                   g_esp32Ctx.weather.todayTempMax,
                   g_esp32Ctx.weather.todayTempMin);
            g_esp32Ctx.weatherReady = 0U;
        }
        else if(evt == ESP32_EVT_FETCH_STATUS)
        {
            printf("[ESP32] FETCH_WEATHER=%s\r\n", g_esp32Ctx.fetchWeatherStatus);
        }
        else if(evt == ESP32_EVT_WIFI_STATUS)
        {
            uint8_t connected_now = 0U;

            if(strcmp(g_esp32Ctx.wifiStatus, "CONNECTED") == 0)
            {
                connected_now = 1U;
            }

            if((g_wifi_status_known == 0U) || (connected_now != g_wifi_connected))
            {
                g_wifi_status_known = 1U;
                g_wifi_connected = connected_now;

                if(g_wifi_connected != 0U)
                {
                    g_wifi_icon_visible = 1U;
                    SetWifiIconVisible(1U);

                    if(g_esp32Ctx.weather.valid != 0U)
                    {
                        UpdateOutdoorTempHumi(g_esp32Ctx.weather.temp,
                                              g_esp32Ctx.weather.humidity,
                                              g_esp32Ctx.weather.todayTempMin,
                                              g_esp32Ctx.weather.todayTempMax);
                        UpdateWeatherTextLabels(g_esp32Ctx.weather.text,
                                                g_esp32Ctx.weather.todayTextDay);
                    }
                }
                else
                {
                    g_wifi_disconnect_tick = xTaskGetTickCount();
                    g_wifi_icon_visible = 1U;
                    g_wifi_blink_tick = xTaskGetTickCount();
                    SetWifiIconVisible(1U);
                    UpdateOutdoorTempHumiUnavailable();
                }
            }

            if(g_esp32Ctx.wifiIp[0] != '\0')
            {
                printf("[ESP32] WIFI=%s, IP=%s\r\n", g_esp32Ctx.wifiStatus, g_esp32Ctx.wifiIp);
            }
            else
            {
                printf("[ESP32] WIFI=%s\r\n", g_esp32Ctx.wifiStatus);
            }
        }
    }
}

static void UpdateIndoorTempHumi(float temperature, float humidity)
{
    char temp_text[8];
    char humi_text[8];
    int32_t temp_value = (int32_t)temperature;
    int32_t humi_value = (int32_t)humidity;

    snprintf(temp_text, sizeof(temp_text), "%ld", (long)temp_value);
    snprintf(humi_text, sizeof(humi_text), "%ld", (long)humi_value);

    if(guider_ui.screen_1_spangroup_7_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_7_span, temp_text);
    }

    if(guider_ui.screen_1_spangroup_8_span != NULL)
    {
        lv_span_set_text(guider_ui.screen_1_spangroup_8_span, humi_text);
    }

    if(guider_ui.screen_1_spangroup_7 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_7);
    }

    if(guider_ui.screen_1_spangroup_8 != NULL)
    {
        lv_spangroup_refr_mode(guider_ui.screen_1_spangroup_8);
    }
}

static void LED_Task(void *pvParameters)
{
    (void)pvParameters;

    while(1)
    {
        LED1_Toggle;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void LVGL_Task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t sensor_ready = 0;
    uint8_t init_ret = 0;
    uint8_t esp32_boot_cmd_sent = 0;
    TickType_t last_update_tick = xTaskGetTickCount();
    TickType_t last_sync_tick = xTaskGetTickCount();
    TickType_t last_weather_tick = xTaskGetTickCount();

    g_wifi_connected = 0U;
    g_wifi_status_known = 0U;
    g_wifi_icon_visible = 1U;
    g_wifi_blink_tick = xTaskGetTickCount();
    g_wifi_disconnect_tick = xTaskGetTickCount();
    g_wifi_probe_tick = xTaskGetTickCount();
    SetWifiIconVisible(1U);
    UpdateOutdoorTempHumiUnavailable();

    // UpdateIndoorTempHumi(11.0f, 22.0f);
    // printf("[AHT10] UI通道最小验证: Temp=11, Humi=22\r\n");

    init_ret = AHT10_Init();
    printf("[AHT10] Init ret=%u\r\n", (unsigned int)init_ret);

    if(init_ret == 0)
    {
        sensor_ready = 1;
        last_update_tick -= pdMS_TO_TICKS(10000);
    }
    else
    {
        printf("AHT10初始化失败\r\n");
    }

    while(1)
    {
        TickType_t now_tick = xTaskGetTickCount();

        ProcessEsp32Protocol();
        UpdateWifiIndicatorUi(now_tick);

        if(((g_wifi_status_known == 0U) || (g_wifi_connected == 0U)) &&
           (now_tick - g_wifi_probe_tick >= WIFI_DISCONNECT_PROBE_INTERVAL))
        {
            Esp32_SendCmd("GET_WIFI");
            g_wifi_probe_tick = now_tick;
        }

        if((esp32_boot_cmd_sent == 0U) && (now_tick > pdMS_TO_TICKS(1000)))
        {
            Esp32_SendCmd("PING");
            Esp32_SendCmd("GET_WIFI");
            g_wifi_probe_tick = now_tick;
            Esp32_SendCmd("SYNC_TIME");
            Esp32_SendCmd("GET_WEATHER");
            esp32_boot_cmd_sent = 1U;
            last_sync_tick = now_tick;
            last_weather_tick = now_tick;
        }

        if(now_tick - last_sync_tick >= pdMS_TO_TICKS(86400000))
        {
            Esp32_SendCmd("GET_WIFI");
            g_wifi_probe_tick = now_tick;
            Esp32_SendCmd("SYNC_TIME");
            last_sync_tick = now_tick;
        }

        if(now_tick - last_weather_tick >= pdMS_TO_TICKS(600000))
        {
            Esp32_SendCmd("GET_WIFI");
            g_wifi_probe_tick = now_tick;
            Esp32_SendCmd("FETCH_WEATHER");
            last_weather_tick = now_tick;
        }

        if(sensor_ready && (now_tick - last_update_tick >= pdMS_TO_TICKS(10000)))
        {
            float humidity = 0.0f;
            float temperature = 0.0f;
            uint8_t read_ret = 0;

            read_ret = AHT10_ReadData(&humidity, &temperature);
            if(read_ret == 0)
            {
                UpdateIndoorTempHumi(temperature, humidity);
                printf("[AHT10] Read OK: T=%ld, H=%ld\r\n", (long)((int32_t)temperature), (long)((int32_t)humidity));
            }
            else
            {
                printf("[AHT10] Read Fail ret=%u\r\n", (unsigned int)read_ret);
            }

            last_update_tick = now_tick;
        }

        uint32_t wait_ms = lv_timer_handler();

        if(wait_ms < 1U)
        {
            wait_ms = 1U;
        }
        else if(wait_ms > 20U)
        {
            wait_ms = 20U;
        }

        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}



void LVGL_Test(void) {
    // 按钮
    lv_obj_t *myBtn = lv_btn_create(lv_scr_act());                               // 创建按钮; 父对象：当前活动屏幕
    lv_obj_set_pos(myBtn, 10, 10);                                               // 设置坐标
    lv_obj_set_size(myBtn, 120, 50);                                             // 设置大小
   
    // 按钮上的文本
    lv_obj_t *label_btn = lv_label_create(myBtn);                                // 创建文本标签，父对象：上面的btn按钮
    lv_obj_align(label_btn, LV_ALIGN_CENTER, 0, 0);                              // 对齐于：父对象
    lv_label_set_text(label_btn, "Test");                                        // 设置标签的文本
 
    // 独立的标签
    lv_obj_t *myLabel = lv_label_create(lv_scr_act());                           // 创建文本标签; 父对象：当前活动屏幕
    lv_label_set_text(myLabel, "Hello world!");                                  // 设置标签的文本
    lv_obj_align(myLabel, LV_ALIGN_CENTER, 0, 0);                                // 对齐于：父对象
    lv_obj_align_to(myBtn, myLabel, LV_ALIGN_OUT_TOP_MID, 0, -20);               // 对齐于：某对象
}


int main(void)
{
	//要严格的初始化顺序，
	//不同的初始化顺序可能会导致无法完成初始化，启动系统
    delay_init(168);     // 初始化延时函数
	LED_Init();         // LED初始化
    Usart_Config();     // USART初始化函数
    Esp32Uart_Config();
    Esp32Protocol_Init(&g_esp32Ctx);
	printf("系统时钟: %d MHz\r\n", SystemCoreClock / 1000000);
    printf("开始初始化...\r\n");
    printf("ESP32串口(USART2 PA2/PA3)初始化完成\r\n");
    
    // 1. 屏幕初始化
    LCD_Init();
    printf("屏幕初始化完成\r\n");
    
    // 2. 触屏初始化
    printf("开始触屏初始化...\r\n");
    if(tp_dev.init())
    {
        printf("触屏初始化失败!\r\n");
    }
    else
    {
        printf("触屏初始化成功!\r\n");
    }
    
    // 3. 设置为横屏模式
    LCD_direction(1);
    printf("设置横屏模式完成\r\n");
    
    // 4. 清屏并设置背景
    LCD_Clear(WHITE);
	printf("清屏完成, 颜色: %d\r\n", WHITE);  // 检查 WHITE 值是否为预期 (通常 0xFFFF 为白)
    printf("清屏完成\r\n");
    
    printf("LVGL初始化开始\r\n");
    lv_init();                             // LVGL 初始化
    lv_port_disp_init();                   // 注册LVGL的显示任务
    lv_port_indev_init();                  // 注册LVGL的触屏检测任务
    printf("LVGL初始化完成\r\n");
    
    printf("全部初始化完成…………\r\n");
    
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);

    if(xTaskCreate(LVGL_Task,
                   "LVGL",
                   1024,
                   NULL,
                   configMAX_PRIORITIES - 2,
                   NULL) != pdPASS)
    {
        printf("LVGL任务创建失败!\r\n");
        while(1)
        {
        }
    }

    if(xTaskCreate(LED_Task,
                   "LED",
                   128,
                   NULL,
                   tskIDLE_PRIORITY + 1,
                   NULL) != pdPASS)
    {
        printf("LED任务创建失败!\r\n");
        while(1)
        {
        }
    }

    vTaskStartScheduler();

    while(1)
    {
    }
}

