#include "fd_timing.h"

#include <zephyr/sys_clock.h>

#include <soc.h>

void fd_timing_enable(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL |= 0x01;
    DWT->CYCCNT = 0;
}

void fd_timing_disable(void) {
    DWT->CTRL &= ~0x01;

    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

uint32_t fd_timing_get_timestamp(void) {
    return DWT->CYCCNT;
}

double fd_timing_get_us_per_timestamp(void) {
    double rate = (double)sys_clock_hw_cycles_per_sec() / 1000000.0;
    return 1.0 / rate;
}