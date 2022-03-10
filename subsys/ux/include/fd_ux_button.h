#ifndef fd_ux_button_h
#define fd_ux_button_h

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float timestamp;
    uint32_t buttons_0;
    uint32_t buttons_1;
    uint32_t buttons_n;
    uint32_t changes;
} fd_ux_button_event_t;

typedef void (*fd_ux_button_callback_t)(const fd_ux_button_event_t *event);

typedef struct {
    const fd_gpio_t *gpios;
    uint32_t count;
    fd_ux_button_callback_t callback;
} fd_ux_button_configuration_t;

void fd_ux_button_initialize(const fd_ux_button_configuration_t *configuration);

bool fd_ux_button_is_activated(const fd_ux_button_event_t *event, uint32_t mask);
bool fd_ux_button_is_one_activated(const fd_ux_button_event_t *event, uint32_t mask);
bool fd_ux_button_is_any_activated(const fd_ux_button_event_t *event, uint32_t mask);
bool fd_ux_button_is_all_activated(const fd_ux_button_event_t *event, uint32_t mask);

bool fd_ux_button_is_deactivated(const fd_ux_button_event_t *event, uint32_t mask);

#endif
