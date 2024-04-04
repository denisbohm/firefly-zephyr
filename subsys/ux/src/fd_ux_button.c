#include "fd_ux_button.h"

#include "fd_assert.h"
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
    return ux_button->buttons != 0;
}

void fd_ux_button_ux_state_changed(void *context, fd_ux_state_t old, fd_ux_state_t new) {
    fd_ux_button_t *ux_button = context;
    ux_button->consume_release = fd_ux_button_is_any_pressed(ux_button);
}

void fd_ux_button_event(fd_ux_button_t *ux_button, const fd_button_event_t *event) {
    if (ux_button->consume_release) {
        if (ux_button->buttons == 0) {
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

void fd_ux_button_send(fd_ux_button_t *ux_button, fd_button_type_t type, uint32_t buttons, uint32_t holds, uint32_t chords, float timestamp, float duration) {
    fd_button_event_t event = {
        .type = type,
        .buttons = buttons,
        .holds = holds,
        .chords = chords,
        .timestamp = timestamp,
        .duration = duration,
    };

    if (ux_button->configuration.callback != NULL) {
        ux_button->configuration.callback(ux_button, &event);
    } else {
        fd_ux_button_event(ux_button, &event);
    }
}

void fd_ux_button_check(fd_ux_button_t *ux_button, uint32_t buttons) {
    uint32_t pressed = buttons & ~ux_button->buttons;
    uint32_t released = ~buttons & ux_button->buttons;

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        uint32_t button = 1 << i;
        if ((button & pressed) != 0) {
            state->press_timestamp = fd_timer_get_timestamp();
        }
        if ((button & released) != 0) {
            state->release_timestamp = fd_timer_get_timestamp();
        }
    }

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        if (state->press_timestamp != 0) {
            state->chords |= buttons;
        }
    }
}

void fd_ux_button_timeout(void *context) {
    fd_ux_button_t *ux_button = context;

    // check to see if buttons where pressed or released after debouncing
    uint32_t buttons = fd_ux_read_buttons(ux_button);
    fd_ux_button_check(ux_button, buttons);
    uint32_t holds = buttons | ux_button->buttons;
    ux_button->buttons = buttons;

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        uint32_t button = 1 << i;
        uint32_t chords = state->chords & ~button;
        if ((state->press_timestamp != 0) && !state->press_sent) {
            state->press_sent = true;
            fd_ux_button_send(ux_button, fd_button_type_pressed, button, holds & ~button, chords, state->press_timestamp, 0.0f);
        }
        if (state->release_timestamp != 0) {
            float timestamp = state->release_timestamp;
            float duration = timestamp - state->press_timestamp;
            state->press_timestamp = 0;
            state->press_sent = false;
            state->release_timestamp = 0;
            state->chords = 0;
            fd_ux_button_send(ux_button, fd_button_type_released, button, holds & ~button, chords, timestamp, duration);
        }
    }
}

void fd_ux_button_change(void *context) {
    fd_ux_button_t *ux_button = context;

    // ignore changes during debounce time
    if (ux_button->timer.active) {
        return;
    }

    uint32_t buttons = fd_ux_read_buttons(ux_button);
    fd_ux_button_check(ux_button, buttons);
    ux_button->buttons = buttons;

    // start debounce timer
    fd_timer_start(&ux_button->timer, 0.05);
}

void fd_ux_button_initialize(fd_ux_button_t *ux_button, const fd_ux_button_configuration_t *configuration) {
    fd_assert(configuration->count <= CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT);

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

    ux_button->listener = (fd_ux_listener_t) {
        .context = ux_button,
        .state_changed = fd_ux_button_ux_state_changed,
    };
    fd_ux_add_listener(ux_button->configuration.ux, &ux_button->listener);
}
