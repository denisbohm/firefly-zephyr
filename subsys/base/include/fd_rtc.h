#ifndef fd_rtc_h
#define fd_rtc_h

#include <stdbool.h>
#include <stdint.h>

void fd_rtc_initialize(void);

bool fd_rtc_is_set(void);
void fd_rtc_set_utc(int64_t utc);

int64_t fd_rtc_get_utc(void);

#endif
