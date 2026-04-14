#ifndef WEATHERCLOCK_UI_TIME_DISPLAY_H
#define WEATHERCLOCK_UI_TIME_DISPLAY_H

#include <stddef.h>

#include "gui_guider.h"
#include "usart.h"

#ifdef __cplusplus
extern "C" {
#endif

void UiTimeDisplay_Init(lv_ui *ui);
void UiTimeDisplay_UpdateByTimeSync(const TimeSyncInfo *timeSync,
                                    int *hour_value,
                                    int *min_value,
                                    int *sec_value,
                                    char *meridiem_buf,
                                    size_t meridiem_size);

#ifdef __cplusplus
}
#endif

#endif
