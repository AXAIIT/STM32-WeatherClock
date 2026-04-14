#ifndef WEATHERCLOCK_UI_ENV_DISPLAY_H
#define WEATHERCLOCK_UI_ENV_DISPLAY_H

#include "gui_guider.h"

#ifdef __cplusplus
extern "C" {
#endif

void UiEnvDisplay_Init(lv_ui *ui);
void UiEnvDisplay_UpdateOutdoor(int temp, int humidity, int todayMin, int todayMax);
void UiEnvDisplay_SetOutdoorUnavailable(void);
void UiEnvDisplay_UpdateIndoor(float temperature, float humidity);

#ifdef __cplusplus
}
#endif

#endif
