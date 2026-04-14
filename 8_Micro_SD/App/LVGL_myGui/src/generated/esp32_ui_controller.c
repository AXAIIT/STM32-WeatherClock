#include "esp32_ui_controller.h"

#include <stdio.h>
#include <string.h>

#include "ui_env_display.h"
#include "ui_time_display.h"
#include "weather_display.h"

static lv_ui *s_ui = NULL;
static Esp32ProtocolContext *s_ctx = NULL;
static int *s_hour_value = NULL;
static int *s_min_value = NULL;
static int *s_sec_value = NULL;
static char *s_meridiem_buf = NULL;
static size_t s_meridiem_size = 0U;

static uint8_t *s_wifi_connected = NULL;
static uint8_t *s_wifi_status_known = NULL;
static uint8_t *s_wifi_icon_visible = NULL;
static TickType_t *s_wifi_blink_tick = NULL;
static TickType_t *s_wifi_disconnect_tick = NULL;
static TickType_t *s_wifi_probe_tick = NULL;

static const TickType_t WIFI_BLINK_INTERVAL = pdMS_TO_TICKS(500);
static const TickType_t WIFI_DISCONNECT_BLINK_DELAY = pdMS_TO_TICKS(30000);

static void SetWifiIconVisible(uint8_t visible)
{
    if((s_ui == NULL) || (s_ui->screen_1_img_2 == NULL))
    {
        return;
    }

    if(visible != 0U)
    {
        lv_obj_clear_flag(s_ui->screen_1_img_2, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(s_ui->screen_1_img_2, LV_OBJ_FLAG_HIDDEN);
    }
}

void Esp32UiController_Init(lv_ui *ui,
                            Esp32ProtocolContext *ctx,
                            int *hour_value,
                            int *min_value,
                            int *sec_value,
                            char *meridiem_buf,
                            size_t meridiem_size,
                            uint8_t *wifi_connected,
                            uint8_t *wifi_status_known,
                            uint8_t *wifi_icon_visible,
                            TickType_t *wifi_blink_tick,
                            TickType_t *wifi_disconnect_tick,
                            TickType_t *wifi_probe_tick)
{
    s_ui = ui;
    s_ctx = ctx;
    s_hour_value = hour_value;
    s_min_value = min_value;
    s_sec_value = sec_value;
    s_meridiem_buf = meridiem_buf;
    s_meridiem_size = meridiem_size;

    s_wifi_connected = wifi_connected;
    s_wifi_status_known = wifi_status_known;
    s_wifi_icon_visible = wifi_icon_visible;
    s_wifi_blink_tick = wifi_blink_tick;
    s_wifi_disconnect_tick = wifi_disconnect_tick;
    s_wifi_probe_tick = wifi_probe_tick;
}

void Esp32UiController_UpdateWifiIndicatorUi(TickType_t now_tick)
{
    if((s_wifi_status_known == NULL) || (s_wifi_connected == NULL) ||
       (s_wifi_icon_visible == NULL) || (s_wifi_blink_tick == NULL) ||
       (s_wifi_disconnect_tick == NULL))
    {
        return;
    }

    if(*s_wifi_status_known == 0U)
    {
        return;
    }

    if(*s_wifi_connected != 0U)
    {
        if(*s_wifi_icon_visible == 0U)
        {
            *s_wifi_icon_visible = 1U;
            SetWifiIconVisible(1U);
        }
        return;
    }

    if(now_tick - *s_wifi_disconnect_tick < WIFI_DISCONNECT_BLINK_DELAY)
    {
        if(*s_wifi_icon_visible == 0U)
        {
            *s_wifi_icon_visible = 1U;
            SetWifiIconVisible(1U);
        }
        return;
    }

    if(now_tick - *s_wifi_blink_tick >= WIFI_BLINK_INTERVAL)
    {
        *s_wifi_blink_tick = now_tick;
        *s_wifi_icon_visible = (*s_wifi_icon_visible == 0U) ? 1U : 0U;
        SetWifiIconVisible(*s_wifi_icon_visible);
    }
}

void Esp32UiController_ForceWifiIconVisible(uint8_t visible)
{
    if(s_wifi_icon_visible != NULL)
    {
        *s_wifi_icon_visible = (visible != 0U) ? 1U : 0U;
    }
    SetWifiIconVisible(visible);
}

void Esp32UiController_ProcessEsp32Protocol(void)
{
    char line[ESP32_UART_LINE_MAX_LEN];

    if((s_ctx == NULL) || (s_wifi_connected == NULL) || (s_wifi_status_known == NULL) ||
       (s_wifi_icon_visible == NULL) || (s_wifi_blink_tick == NULL) ||
       (s_wifi_disconnect_tick == NULL) || (s_wifi_probe_tick == NULL))
    {
        return;
    }

    while(Esp32Uart_GetLine(line, (uint16_t)sizeof(line)) != 0U)
    {
        Esp32ParseEvent evt = Esp32Protocol_ParseLine(s_ctx, line);

        if(evt == ESP32_EVT_PONG)
        {
            printf("[ESP32] PONG\r\n");
        }
        else if(evt == ESP32_EVT_TIME_SYNC)
        {
            UiTimeDisplay_UpdateByTimeSync(&s_ctx->timeSync,
                                           s_hour_value,
                                           s_min_value,
                                           s_sec_value,
                                           s_meridiem_buf,
                                           s_meridiem_size);
            printf("[ESP32] TIME_SYNC %04d-%02d-%02d %02d:%02d:%02d\r\n",
                   s_ctx->timeSync.year,
                   s_ctx->timeSync.month,
                   s_ctx->timeSync.day,
                   s_ctx->timeSync.hour,
                   s_ctx->timeSync.min,
                   s_ctx->timeSync.sec);
            s_ctx->timeSyncReady = 0U;
        }
        else if(evt == ESP32_EVT_WEATHER_READY)
        {
            UiEnvDisplay_UpdateOutdoor(s_ctx->weather.temp,
                                       s_ctx->weather.humidity,
                                       s_ctx->weather.todayTempMin,
                                       s_ctx->weather.todayTempMax);
            WeatherDisplay_Update(&s_ctx->weather);
            printf("[ESP32] WEATHER temp=%d humi=%d press=%d todayMax=%d todayMin=%d\r\n",
                   s_ctx->weather.temp,
                   s_ctx->weather.humidity,
                   s_ctx->weather.pressure,
                   s_ctx->weather.todayTempMax,
                   s_ctx->weather.todayTempMin);
            s_ctx->weatherReady = 0U;
        }
        else if(evt == ESP32_EVT_FETCH_STATUS)
        {
            printf("[ESP32] FETCH_WEATHER=%s\r\n", s_ctx->fetchWeatherStatus);
        }
        else if(evt == ESP32_EVT_WIFI_STATUS)
        {
            uint8_t connected_now = 0U;

            if(strcmp(s_ctx->wifiStatus, "CONNECTED") == 0)
            {
                connected_now = 1U;
            }

            if((*s_wifi_status_known == 0U) || (connected_now != *s_wifi_connected))
            {
                *s_wifi_status_known = 1U;
                *s_wifi_connected = connected_now;

                if(*s_wifi_connected != 0U)
                {
                    *s_wifi_icon_visible = 1U;
                    SetWifiIconVisible(1U);

                    if(s_ctx->weather.valid != 0U)
                    {
                        UiEnvDisplay_UpdateOutdoor(s_ctx->weather.temp,
                                                   s_ctx->weather.humidity,
                                                   s_ctx->weather.todayTempMin,
                                                   s_ctx->weather.todayTempMax);
                        WeatherDisplay_Update(&s_ctx->weather);
                    }
                }
                else
                {
                    *s_wifi_disconnect_tick = xTaskGetTickCount();
                    *s_wifi_icon_visible = 1U;
                    *s_wifi_blink_tick = xTaskGetTickCount();
                    SetWifiIconVisible(1U);
                    UiEnvDisplay_SetOutdoorUnavailable();
                }
            }

            if(s_ctx->wifiIp[0] != '\0')
            {
                printf("[ESP32] WIFI=%s, IP=%s\r\n", s_ctx->wifiStatus, s_ctx->wifiIp);
            }
            else
            {
                printf("[ESP32] WIFI=%s\r\n", s_ctx->wifiStatus);
            }
        }
    }
}
