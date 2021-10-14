#include "fd_delay.h"

#include "zephyr.h"

void fd_delay_ms(uint32_t ms) {
    k_busy_wait(ms * 1000);
}

void fd_delay_us(uint32_t us) {
    k_busy_wait(us);
}

void fd_delay_ns(uint32_t ns) {
    k_busy_wait((ns + 999) / 1000);
}
