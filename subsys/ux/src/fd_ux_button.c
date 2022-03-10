#include "fd_ux_button.h"

#include "fd_timer.h"

#include <string.h>

typedef struct {
    fd_ux_button_configuration_t configuration;
    fd_timer_t timer;
    uint32_t buttons;
    fd_ux_button_event_t event;
} fd_ux_button_t;

fd_ux_button_t fd_ux_button;

bool fd_ux_button_is_activated(const fd_ux_button_event_t *event, uint32_t mask) {
    return ((event->buttons_0 & mask) == 0) && (((event->buttons_n | event->changes) & mask) != 0);
}

bool fd_ux_button_is_deactivated(const fd_ux_button_event_t *event, uint32_t mask) {
    return ((event->buttons_0 & mask) == mask) && (((event->buttons_n | event->changes) & mask) == 0);
}

bool fd_ux_button_is_one_activated(const fd_ux_button_event_t *event, uint32_t mask) {
    return (event->buttons_0 == 0) && ((event->buttons_n | event->changes) == mask);
}

bool fd_ux_button_is_any_activated(const fd_ux_button_event_t *event, uint32_t mask) {
    for (uint32_t i = 0; i < fd_ux_button.configuration.count; ++i) {
        uint32_t one_mask = 1 << i;
        if ((mask && one_mask) == 0) {
            continue;
        }
        if (fd_ux_button_is_activated(event, one_mask)) {
            return true;
        }
    }
    return false;
}

bool fd_ux_button_is_all_activated(const fd_ux_button_event_t *event, uint32_t mask) {
    for (uint32_t i = 0; i < fd_ux_button.configuration.count; ++i) {
        uint32_t one_mask = 1 << i;
        if ((mask && one_mask) == 0) {
            continue;
        }
        if (!fd_ux_button_is_activated(event, one_mask)) {
            return false;
        }
    }
    return true;
}

uint32_t fd_ux_read_buttons(void) {
    uint32_t buttons = 0;
    for (uint32_t i = 0; i < fd_ux_button.configuration.count; ++i) {
        fd_gpio_t gpio = fd_ux_button.configuration.gpios[i];
        if (!fd_gpio_get(gpio)) {
            buttons |= (1 << i);
        }
    }
    return buttons;
}

void fd_ux_button_changed(void) {
    uint32_t buttons = fd_ux_read_buttons();
    fd_ux_button_event_t *event = &fd_ux_button.event;
    if (!fd_ux_button.timer.active) {
        event->timestamp = fd_timer_get_timestamp();
        event->buttons_0 = fd_ux_button.buttons;
        event->buttons_1 = buttons;
        event->buttons_n = buttons;
        event->changes = 0;
        fd_timer_start(&fd_ux_button.timer, 0.05);
    } else {
        event->buttons_n = buttons;
        event->changes |= event->buttons_1 ^ buttons;
    }
    fd_ux_button.buttons = buttons;
}

void fd_ux_button_timeout(void) {
    fd_ux_button_event_t event = fd_ux_button.event;
    memset(&fd_ux_button.event, 0, sizeof(fd_ux_button.event));
    fd_ux_button.configuration.callback(&event);
}

void fd_ux_button_initialize(const fd_ux_button_configuration_t *configuration) {
    memset(&fd_ux_button, 0, sizeof(fd_ux_button));
    fd_ux_button.configuration = *configuration;
    
    fd_timer_add(&fd_ux_button.timer, fd_ux_button_timeout);

    for (uint32_t i = 0; i < fd_ux_button.configuration.count; ++i) {
        fd_gpio_t gpio = fd_ux_button.configuration.gpios[i];
        fd_gpio_set_callback(gpio, fd_gpio_edge_both, fd_ux_button_changed);
    }
}
