#include "ui_time_display.h"

#include <string.h>

#include "custom.h"

static lv_ui *s_ui = NULL;

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

void UiTimeDisplay_Init(lv_ui *ui)
{
    s_ui = ui;
}

void UiTimeDisplay_UpdateByTimeSync(const TimeSyncInfo *timeSync,
                                    int *hour_value,
                                    int *min_value,
                                    int *sec_value,
                                    char *meridiem_buf,
                                    size_t meridiem_size)
{
    int hour24;
    int hour12;
    int weekdayIndex;

    if((s_ui == NULL) || (timeSync == NULL) || (timeSync->valid == 0U) ||
       (hour_value == NULL) || (min_value == NULL) || (sec_value == NULL) ||
       (meridiem_buf == NULL) || (meridiem_size < 3U))
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
        meridiem_buf[0] = 'A';
        meridiem_buf[1] = 'M';
        meridiem_buf[2] = '\0';
    }
    else if(hour24 < 12)
    {
        hour12 = hour24;
        meridiem_buf[0] = 'A';
        meridiem_buf[1] = 'M';
        meridiem_buf[2] = '\0';
    }
    else if(hour24 == 12)
    {
        hour12 = 12;
        meridiem_buf[0] = 'P';
        meridiem_buf[1] = 'M';
        meridiem_buf[2] = '\0';
    }
    else
    {
        hour12 = hour24 - 12;
        meridiem_buf[0] = 'P';
        meridiem_buf[1] = 'M';
        meridiem_buf[2] = '\0';
    }

    *hour_value = hour12;
    *min_value = timeSync->min;
    *sec_value = timeSync->sec;

    if(s_ui->screen_1_digital_clock_1 != NULL)
    {
        lv_dclock_set_text_fmt(s_ui->screen_1_digital_clock_1,
                               "%d:%02d %s",
                               *hour_value,
                               *min_value,
                               meridiem_buf);
    }

    if(s_ui->screen_1_datetext_1 != NULL)
    {
        lv_label_set_text_fmt(s_ui->screen_1_datetext_1,
                              "%d/%02d/%02d",
                              timeSync->year,
                              timeSync->month,
                              timeSync->day);
    }

    weekdayIndex = CalcWeekdayMonday0(timeSync->year, timeSync->month, timeSync->day);
    custom_set_weekday_text(s_ui, weekdayIndex);
}
