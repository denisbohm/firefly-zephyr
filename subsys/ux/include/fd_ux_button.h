#ifndef fd_ux_button_h
#define fd_ux_button_h

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fd_ux_button_type_pressed,
    fd_ux_button_type_released,
} fd_ux_button_type_t;

typedef struct {
    fd_ux_button_type_t type;
    uint32_t buttons;
    float timestamp;
    float duration;
} fd_ux_button_event_t;

typedef void (*fd_ux_button_callback_t)(const fd_ux_button_event_t *event);

typedef struct {
    const fd_gpio_t *gpios;
    uint32_t count;
    fd_ux_button_callback_t callback;
} fd_ux_button_configuration_t;

void fd_ux_button_initialize(const fd_ux_button_configuration_t *configuration);

bool fd_ux_button_was_pressed(const fd_ux_button_event_t *event, uint32_t mask);
bool fd_ux_button_was_released(const fd_ux_button_event_t *event, uint32_t mask);

bool fd_ux_button_is_any_pressed(void);

#endif
