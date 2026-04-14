#include "ui_env_display.h"

#include <stdio.h>
#include <stdint.h>

#include "lvgl.h"
#include "weather_display.h"

static lv_ui *s_ui = NULL;

void UiEnvDisplay_Init(lv_ui *ui)
{
    s_ui = ui;
}

void UiEnvDisplay_UpdateOutdoor(int temp, int humidity, int todayMin, int todayMax)
{
    char temp_text[8];
    char humi_text[8];
    char low_text[8];
    char high_text[8];

    if(s_ui == NULL)
    {
        return;
    }

    snprintf(temp_text, sizeof(temp_text), "%d", temp);
    snprintf(humi_text, sizeof(humi_text), "%d", humidity);
    snprintf(low_text, sizeof(low_text), "%d", todayMin);
    snprintf(high_text, sizeof(high_text), "%d", todayMax);

    if(s_ui->screen_1_spangroup_10_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_10_span, temp_text);
    }

    if(s_ui->screen_1_spangroup_11_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_11_span, humi_text);
    }

    if(s_ui->screen_1_spangroup_13_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_13_span, low_text);
    }

    if(s_ui->screen_1_spangroup_14_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_14_span, high_text);
    }

    if(s_ui->screen_1_spangroup_10 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_10);
    }

    if(s_ui->screen_1_spangroup_11 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_11);
    }

    if(s_ui->screen_1_spangroup_13 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_13);
    }

    if(s_ui->screen_1_spangroup_14 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_14);
    }
}

void UiEnvDisplay_SetOutdoorUnavailable(void)
{
    if(s_ui == NULL)
    {
        return;
    }

    if(s_ui->screen_1_spangroup_10_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_10_span, "--");
    }

    if(s_ui->screen_1_spangroup_11_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_11_span, "--");
    }

    if(s_ui->screen_1_spangroup_10 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_10);
    }

    if(s_ui->screen_1_spangroup_11 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_11);
    }

    WeatherDisplay_Update(NULL);
}

void UiEnvDisplay_UpdateIndoor(float temperature, float humidity)
{
    char temp_text[8];
    char humi_text[8];
    int32_t temp_value = (int32_t)temperature;
    int32_t humi_value = (int32_t)humidity;

    if(s_ui == NULL)
    {
        return;
    }

    snprintf(temp_text, sizeof(temp_text), "%ld", (long)temp_value);
    snprintf(humi_text, sizeof(humi_text), "%ld", (long)humi_value);

    if(s_ui->screen_1_spangroup_7_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_7_span, temp_text);
    }

    if(s_ui->screen_1_spangroup_8_span != NULL)
    {
        lv_span_set_text(s_ui->screen_1_spangroup_8_span, humi_text);
    }

    if(s_ui->screen_1_spangroup_7 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_7);
    }

    if(s_ui->screen_1_spangroup_8 != NULL)
    {
        lv_spangroup_refr_mode(s_ui->screen_1_spangroup_8);
    }
}
