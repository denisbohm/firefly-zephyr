#include "fd_timing.h"

#include "nrf.h"

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
