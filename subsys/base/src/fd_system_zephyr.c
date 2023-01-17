#include "fd_system.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>

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
