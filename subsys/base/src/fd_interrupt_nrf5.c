#include "nrf.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

uint32_t im_interrupt_disable(void) {
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

void im_interrupt_enable(uint32_t state) {
    if (!state) {
        __enable_irq();
    }
}

void im_interrupt_wait(void) {
    __WFI();
}