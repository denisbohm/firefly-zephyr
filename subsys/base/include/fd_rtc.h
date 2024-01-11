#ifndef fd_rtc_h
#define fd_rtc_h

#include <stdbool.h>
#include <stdint.h>

void fd_rtc_initialize(void);

bool fd_rtc_is_set(void);
void fd_rtc_set_utc(int64_t utc);

int64_t fd_rtc_get_utc(void);
double fd_rtc_get_utc_precise(void);

typedef enum {
    fd_rtc_display_format_use_12_hour_clock,
    fd_rtc_display_format_use_24_hour_clock,
} fd_rtc_display_format_t;

typedef struct {
    int32_t time_zone_offset;
    fd_rtc_display_format_t display_format;
} fd_rtc_configuration_t;

void fd_rtc_get_configuration(fd_rtc_configuration_t *configuration);
void fd_rtc_set_configuration(const fd_rtc_configuration_t *configuration);

#endif
