#include "fd_delay.h"
#include "fd_rtc.h"

void main(void) {
    fd_rtc_initialize();
    fd_rtc_set_utc(1636381101); // 8:18 AM CST
    while (1) {
        uint64_t utc = fd_rtc_get_utc();
        fd_delay_ms(5000);
    }
}
