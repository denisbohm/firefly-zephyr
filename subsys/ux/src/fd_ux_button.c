#include "fd_ux_button.h"

#include "fd_timer.h"
#include "fd_ux.h"

#include <string.h>

uint32_t fd_ux_read_buttons(fd_ux_button_t *ux_button) {
    uint32_t buttons = 0;
    for (uint32_t i = 0; i < ux_button->configuration.count; ++i) {
        fd_gpio_t gpio = ux_button->configuration.gpios[i];
        if (!fd_gpio_get(gpio)) {
            buttons |= (1 << i);
        }
    }
    return buttons;
}

bool fd_ux_button_is_any_pressed(fd_ux_button_t *ux_button) {
    return ux_button->pressed.timestamp != 0;
}

void fd_ux_button_ux_state_changed(void *context, fd_ux_state_t old, fd_ux_state_t new) {
    fd_ux_button_t *ux_button = context;
    ux_button->consume_release = fd_ux_button_is_any_pressed(ux_button);
}

void fd_ux_button_event(fd_ux_button_t *ux_button, const fd_button_event_t *event) {
    if (ux_button->consume_release) {
        if (event->type == fd_button_type_released) {
            ux_button->consume_release = false;
        }
        return;
    }

    fd_ux_t *ux = ux_button->configuration.ux;
    if (fd_ux_get_state(ux) != fd_ux_state_on) {
        fd_ux_set_state(ux, fd_ux_state_on);
        return;
    }

    fd_ux_active(ux);
    fd_ux_screen_t *screen = fd_ux_get_active_screen(ux);
    if (screen->button != NULL) {
        screen->button(event);
    }
}

void fd_ux_button_pressed(fd_ux_button_t *ux_button) {
    fd_button_event_t event = {
        .type = fd_button_type_pressed,
        .buttons = ux_button->pressed.buttons,
        .timestamp = ux_button->pressed.timestamp,
    };
    fd_ux_button_event(ux_button, &event);
}

void fd_ux_button_released(fd_ux_button_t *ux_button) {
    fd_button_event_t event = {
        .type = fd_button_type_released,
        .buttons = ux_button->released.buttons,
        .timestamp = ux_button->released.timestamp,
        .duration = ux_button->released.timestamp - ux_button->pressed.timestamp,
    };
    memset(&ux_button->pressed, 0, sizeof(ux_button->pressed));
    memset(&ux_button->released, 0, sizeof(ux_button->released));
    fd_ux_button_event(ux_button, &event);
}

void fd_ux_button_timeout(void *context) {
    fd_ux_button_t *ux_button = context;

    if (ux_button->released.timestamp == 0) {
        // press debounce timer done
        fd_ux_button_pressed(ux_button);

        uint32_t buttons = fd_ux_read_buttons(ux_button);
        if (buttons == 0) {
            // released during debounce time
            ux_button->released.timestamp = fd_timer_get_timestamp();
            ux_button->released.buttons = ux_button->pressed.buttons;
            fd_ux_button_released(ux_button);
        }
    } else {
        // release debounce timer done
        fd_ux_button_released(ux_button);

        uint32_t buttons = fd_ux_read_buttons(ux_button);
        if (buttons != 0) {
            // pressed during debounce time
            ux_button->pressed.timestamp = fd_timer_get_timestamp();
            ux_button->pressed.buttons = buttons;
            fd_ux_button_pressed(ux_button);
        }
    }
}

void fd_ux_button_change(void *context) {
    fd_ux_button_t *ux_button = context;

    // ignore changes during debounce time
    if (ux_button->timer.active) {
        return;
    }

    // start debounce timer
    fd_timer_start(&ux_button->timer, 0.05);

    if (ux_button->pressed.timestamp == 0) {
        // press
        ux_button->pressed.timestamp = fd_timer_get_timestamp();
        ux_button->pressed.buttons = fd_ux_read_buttons(ux_button);
    } else {
        // release
        ux_button->released.timestamp = fd_timer_get_timestamp();
        ux_button->released.buttons = ux_button->pressed.buttons;
    }
}

void fd_ux_button_initialize(fd_ux_button_t *ux_button, const fd_ux_button_configuration_t *configuration) {
    memset(ux_button, 0, sizeof(*ux_button));
    ux_button->configuration = *configuration;
    
    ux_button->timer.context = ux_button;
    fd_timer_add(&ux_button->timer, fd_ux_button_timeout);

    for (uint32_t i = 0; i < ux_button->configuration.count; ++i) {
        fd_gpio_t gpio = ux_button->configuration.gpios[i];
        fd_gpio_listener_t listener = {
            .context = ux_button,
            .change = fd_ux_button_change,
        };
        fd_gpio_set_listener(gpio, fd_gpio_edge_both, listener);
    }

    const static fd_ux_listener_t listener = {
        .state_changed = fd_ux_button_ux_state_changed,
    };
    fd_ux_add_listener(ux_button->configuration.ux, &listener);
}
