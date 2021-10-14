#ifndef fd_calendar_h
#define fd_calendar_h

#include <stdint.h>

typedef struct {
    int year;  // years since 1900
    int wday;  // day of week [0,6] (Sunday = 0)
    int month; // month of year [0,11]
    int mday;  // day of month [1,31]
    int hour;  // hour [0,23]
    int min;   // minutes [0,59]
    int sec;   // seconds [0,59]
} fd_calendar_t;

typedef struct {
    char meridiem; // 'a' or 'p'
    unsigned hour;
} fd_calendar_12_hour_t;

int32_t fd_calendar_to_time(fd_calendar_t calendar);
fd_calendar_t fd_calendar_from_time(int32_t time);

fd_calendar_12_hour_t fd_calendar_get_12_hour(const fd_calendar_t *date_time);

#endif
