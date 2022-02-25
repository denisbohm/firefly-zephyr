#include "nrf.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

uint32_t fd_interrupt_disable(void) {
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

void fd_interrupt_enable(uint32_t state) {
    if (!state) {
        __enable_irq();
    }
}

void fd_interrupt_wait(void) {
    __WFI();
}