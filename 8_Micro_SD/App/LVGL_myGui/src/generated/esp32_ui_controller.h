#ifndef WEATHERCLOCK_ESP32_UI_CONTROLLER_H
#define WEATHERCLOCK_ESP32_UI_CONTROLLER_H

#include <stddef.h>

#include "gui_guider.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

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
                            TickType_t *wifi_probe_tick);

void Esp32UiController_ProcessEsp32Protocol(void);
void Esp32UiController_UpdateWifiIndicatorUi(TickType_t now_tick);
void Esp32UiController_ForceWifiIconVisible(uint8_t visible);

#ifdef __cplusplus
}
#endif

#endif
