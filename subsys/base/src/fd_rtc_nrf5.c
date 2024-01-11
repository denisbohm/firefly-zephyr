#include "fd_rtc.h"

#include <zephyr/kernel.h>

#include "nrf.h"

#include <string.h>

#define fd_rtc_seconds_per_overflow 512

typedef struct {
    NRF_RTC_Type *nrf_rtc;
    bool is_set;
    volatile int64_t utc_base;
    fd_rtc_configuration_t configuration;
} fd_rtc_t;

fd_rtc_t fd_rtc;

ISR_DIRECT_DECLARE(fd_rtc_irq_handler) {
    NRF_RTC_Type *nrf_rtc = fd_rtc.nrf_rtc;
    if (nrf_rtc->EVENTS_OVRFLW) {
        nrf_rtc->EVENTS_OVRFLW = 0;
        fd_rtc.utc_base += fd_rtc_seconds_per_overflow;
    }
    return 0;
}

void fd_rtc_initialize(void) {
    memset(&fd_rtc, 0, sizeof(fd_rtc));

#ifdef NRF52_SERIES
    fd_rtc.nrf_rtc = NRF_RTC0;
#endif
#ifdef NRF53_SERIES
    fd_rtc.nrf_rtc = NRF_RTC0_S;
#endif

    NRF_RTC_Type *nrf_rtc = fd_rtc.nrf_rtc;
    nrf_rtc->PRESCALER = 0;
    nrf_rtc->EVTENSET = RTC_EVTENSET_OVRFLW_Msk;
    nrf_rtc->INTENSET = RTC_INTENSET_OVRFLW_Msk;
    nrf_rtc->TASKS_START = 1;

    IRQ_DIRECT_CONNECT(RTC0_IRQn, 1, fd_rtc_irq_handler, 0);
    irq_enable(RTC0_IRQn);
}

bool fd_rtc_is_set(void) {
    return fd_rtc.is_set;
}

void fd_rtc_set_utc(int64_t utc) {
    NRF_RTC_Type *nrf_rtc = fd_rtc.nrf_rtc;
    nrf_rtc->TASKS_STOP = 1;
    nrf_rtc->TASKS_CLEAR = 1;
    fd_rtc.utc_base = utc;
    nrf_rtc->TASKS_START = 1;
    fd_rtc.is_set = true;
}

typedef struct {
    int64_t base;
    uint32_t offset;
} fd_rtc_count_t;

fd_rtc_count_t fd_rtc_get_count(void) {
    NRF_RTC_Type *nrf_rtc = fd_rtc.nrf_rtc;
    nrf_rtc->INTENCLR = RTC_INTENSET_OVRFLW_Msk;
    int64_t base = fd_rtc.utc_base;
    uint32_t offset = nrf_rtc->COUNTER;
    nrf_rtc->INTENSET = RTC_INTENSET_OVRFLW_Msk;
    return (fd_rtc_count_t) { .base = base, .offset = offset };
}

int64_t fd_rtc_get_utc(void) {
    fd_rtc_count_t count = fd_rtc_get_count();
    int64_t utc = count.base + (int64_t)(count.offset >> 15);
    return utc;
}

double fd_rtc_get_utc_precise(void) {
    fd_rtc_count_t count = fd_rtc_get_count();
    double utc = ((double)count.base) + ((double)count.offset) / 32768.0;
    return utc;
}

void fd_rtc_get_configuration(fd_rtc_configuration_t *configuration) {
    memcpy(configuration, &fd_rtc.configuration, sizeof(*configuration));
}

void fd_rtc_set_configuration(const fd_rtc_configuration_t *configuration) {
    memcpy(&fd_rtc.configuration, configuration, sizeof(*configuration));
}