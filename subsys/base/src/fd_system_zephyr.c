#include "fd_system.h"

#include "fd_assert.h"

#include <zephyr.h>
#include <device.h>
#include <drivers/hwinfo.h>

#include <nrfx_clock.h>

#if 0
static int fd_system_nrf53_setup(const struct device *unused) {
    ARG_UNUSED(unused);

    nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
    return 0;
}

SYS_INIT(fd_system_nrf53_setup, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

uint32_t fd_system_get_reset_cause(void) {
    uint32_t cause = 0;
    int result = hwinfo_get_reset_cause(&cause);
    fd_assert(result == 0);
    return cause;
}

void fd_system_clear_reset_cause(void) {
    int result = hwinfo_clear_reset_cause();
    fd_assert(result == 0);
}
