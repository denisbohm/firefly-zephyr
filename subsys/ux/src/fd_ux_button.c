#include "fd_ux_button.h"

#include "fd_timer.h"

#include <string.h>

typedef struct {
    uint32_t timestamp;
    uint32_t buttons;
} fd_ux_button_change_t;

typedef struct {
    fd_ux_button_configuration_t configuration;
    fd_timer_t timer;
    fd_ux_button_change_t pressed;
    fd_ux_button_change_t released;
} fd_ux_button_t;

fd_ux_button_t fd_ux_button;

bool fd_ux_button_was_pressed(const fd_ux_button_event_t *event, uint32_t mask) {
    return (event->type == fd_ux_button_type_pressed) && ((event->buttons & mask) != 0);
}

bool fd_ux_button_was_released(const fd_ux_button_event_t *event, uint32_t mask) {
    return (event->type == fd_ux_button_type_released) && ((event->buttons & mask) != 0);
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

bool fd_ux_button_is_any_pressed(void) {
    return fd_ux_button.pressed.timestamp != 0;
}

void fd_ux_button_pressed(void) {
    fd_ux_button_event_t event = {
        .type = fd_ux_button_type_pressed,
        .buttons = fd_ux_button.pressed.buttons,
        .timestamp = fd_ux_button.pressed.timestamp,
    };
    fd_ux_button.configuration.callback(&event);
}

void fd_ux_button_released(void) {
    fd_ux_button_event_t event = {
        .type = fd_ux_button_type_released,
        .buttons = fd_ux_button.released.buttons,
        .timestamp = fd_ux_button.released.timestamp,
        .duration = fd_ux_button.released.timestamp - fd_ux_button.pressed.timestamp,
    };
    memset(&fd_ux_button.pressed, 0, sizeof(fd_ux_button.pressed));
    memset(&fd_ux_button.released, 0, sizeof(fd_ux_button.released));
    fd_ux_button.configuration.callback(&event);
}

void fd_ux_button_timeout(void) {
    if (fd_ux_button.released.timestamp == 0) {
        // press debounce timer done
        fd_ux_button_pressed();

        uint32_t buttons = fd_ux_read_buttons();
        if (buttons == 0) {
            // released during debounce time
            fd_ux_button.released.timestamp = fd_timer_get_timestamp();
            fd_ux_button.released.buttons = fd_ux_button.pressed.buttons;
            fd_ux_button_released();
        }
    } else {
        // release debounce timer done
        fd_ux_button_released();

        uint32_t buttons = fd_ux_read_buttons();
        if (buttons != 0) {
            // pressed during debounce time
            fd_ux_button.pressed.timestamp = fd_timer_get_timestamp();
            fd_ux_button.pressed.buttons = buttons;
            fd_ux_button_pressed();
        }
    }
}

void fd_ux_button_changed(void) {
    // ignore changes during debounce time
    if (fd_ux_button.timer.active) {
        return;
    }

    // start debounce timer
    fd_timer_start(&fd_ux_button.timer, 0.05);

    if (fd_ux_button.pressed.timestamp == 0) {
        // press
        fd_ux_button.pressed.timestamp = fd_timer_get_timestamp();
        fd_ux_button.pressed.buttons = fd_ux_read_buttons();
    } else {
        // release
        fd_ux_button.released.timestamp = fd_timer_get_timestamp();
        fd_ux_button.released.buttons = fd_ux_button.pressed.buttons;
    }
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
