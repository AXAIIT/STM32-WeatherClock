#ifndef WEATHERCLOCK_WEATHER_DISPLAY_H
#define WEATHERCLOCK_WEATHER_DISPLAY_H

#include "gui_guider.h"
#include "usart.h"

#ifdef __cplusplus
extern "C" {
#endif

void WeatherDisplay_Init(lv_ui *ui);
void WeatherDisplay_Update(const WeatherInfo *weather);

#ifdef __cplusplus
}
#endif

#endif
