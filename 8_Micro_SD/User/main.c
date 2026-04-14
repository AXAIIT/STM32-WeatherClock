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
#include "weather_display.h"
#include "ui_env_display.h"
#include "ui_time_display.h"
#include "esp32_ui_controller.h"

#include <stdio.h>
#include <string.h>

#include "lvgl.h"                // 它为整个LVGL提供了更完整的头文件引用
#include "lv_port_disp.h"        // LVGL的显示支持
#include "lv_port_indev.h"       // LVGL的触屏支持

#include "FreeRTOS.h"
#include "task.h"

lv_ui guider_ui;
static Esp32ProtocolContext g_esp32Ctx;
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
    Esp32UiController_ForceWifiIconVisible(1U);
    UiEnvDisplay_SetOutdoorUnavailable();

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

        Esp32UiController_ProcessEsp32Protocol();
        Esp32UiController_UpdateWifiIndicatorUi(now_tick);

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
                UiEnvDisplay_UpdateIndoor(temperature, humidity);
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
    WeatherDisplay_Init(&guider_ui);
    UiEnvDisplay_Init(&guider_ui);
    UiTimeDisplay_Init(&guider_ui);
    Esp32UiController_Init(&guider_ui,
                           &g_esp32Ctx,
                           &screen_1_digital_clock_1_hour_value,
                           &screen_1_digital_clock_1_min_value,
                           &screen_1_digital_clock_1_sec_value,
                           screen_1_digital_clock_1_meridiem,
                           3U,
                           &g_wifi_connected,
                           &g_wifi_status_known,
                           &g_wifi_icon_visible,
                           &g_wifi_blink_tick,
                           &g_wifi_disconnect_tick,
                           &g_wifi_probe_tick);

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

