#include "fd_ux_button.h"

#include "fd_assert.h"
#include "fd_timer.h"
#include "fd_unused.h"
#include "fd_ux.h"

#include <string.h>

fd_source_push()

typedef struct {
    fd_ux_button_t *ux_buttons[4];
    uint32_t ux_button_count;
} fd_ux_button_manager_t;

fd_ux_button_manager_t fd_ux_button_manager;

fd_ux_button_t *fd_ux_button_get(uint32_t identifier) {
    for (uint32_t i = 0; i < fd_ux_button_manager.ux_button_count; ++i) {
        fd_ux_button_t *ux_button = fd_ux_button_manager.ux_buttons[i];
        if (ux_button->configuration.identifier == identifier) {
            return ux_button;
        }
    }
    return NULL;
}

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

void fd_ux_button_ux_state_changed(void *context fd_unused, fd_ux_state_t old fd_unused, fd_ux_state_t new fd_unused) {
}

void fd_ux_button_event(fd_ux_button_t *ux_button, const fd_button_event_t *event) {
    fd_ux_t *ux = ux_button->configuration.ux;
    if (fd_ux_get_state(ux) != fd_ux_state_on) {
        if (event->action == fd_button_action_pressed) {
            ux_button->consume_release = true;
            fd_ux_set_state(ux, fd_ux_state_on);
        }
        return;
    }

    if (ux_button->consume_release && (event->action == fd_button_action_released)) {
        ux_button->consume_release = false;
        return;
    }

    fd_ux_active(ux);
    fd_ux_screen_t *screen = fd_ux_get_active_screen(ux);
    if (screen->button != NULL) {
        screen->button(event);
    }
}

typedef void (*fd_ux_button_send_t)(fd_ux_button_t *ux_button, fd_button_action_t action, uint32_t buttons, uint32_t holds, uint32_t chords, float timestamp, float duration);

void fd_ux_button_send(fd_ux_button_t *ux_button, fd_button_action_t action, uint32_t buttons, uint32_t holds, uint32_t chords, float timestamp, float duration) {
    fd_button_event_t event = {
        .action = action,
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

void fd_ux_button_check(fd_ux_button_t *ux_button, uint32_t buttons, float timestamp) {
    uint32_t pressed = buttons & ~ux_button->buttons;
    uint32_t released = ~buttons & ux_button->buttons;

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        uint32_t button = 1 << i;
        if ((button & pressed) != 0) {
            state->press_timestamp = timestamp;
        }
        if ((button & released) != 0) {
            state->release_timestamp = timestamp;
        }
    }

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        if (state->press_timestamp != 0.0f) {
            state->chords |= buttons;
        }
    }
}

void fd_ux_button_timeout_logic(fd_ux_button_t *ux_button, uint32_t buttons, float timestamp, fd_ux_button_send_t send) {
    uint32_t holds = buttons | ux_button->buttons;
    uint32_t pressed = buttons & ~ux_button->buttons;

    // check to see if buttons where pressed or released after debouncing
    fd_ux_button_check(ux_button, buttons, timestamp);
    ux_button->buttons = buttons;

    for (uint32_t i = 0; i < CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT; ++i) {
        fd_ux_button_state_t *state = &ux_button->states[i];
        uint32_t button = 1 << i;
        uint32_t chords = state->chords & ~button;
        if (state->press_timestamp != 0.0f) {
            if (state->press_sent && ((button & pressed) != 0)) {
                // button still held, send release for that and then start a new press
                continue;
                #if 0
                float timestamp = state->release_timestamp;
                float duration = timestamp - state->press_timestamp;
                state->press_timestamp = timestamp;
                state->press_sent = false;
                state->release_timestamp = 0.0f;
                state->chords = 0;
                send(ux_button, fd_button_action_released, button, holds & ~button, chords, timestamp, duration);
                #endif
            }
            if (!state->press_sent) {
                state->press_sent = true;
                send(ux_button, fd_button_action_pressed, button, holds & ~button, chords, state->press_timestamp, 0.0f);
            }
        }
        if (state->release_timestamp != 0.0f) {
            float timestamp = state->release_timestamp;
            float duration = timestamp - state->press_timestamp;
            state->press_timestamp = 0.0f;
            state->press_sent = false;
            state->release_timestamp = 0.0f;
            state->chords = 0;
            send(ux_button, fd_button_action_released, button, holds & ~button, chords, timestamp, duration);
        }
    }
}

void fd_ux_button_timeout(void *context) {
    fd_ux_button_t *ux_button = context;

    // check to see if buttons where pressed or released after debouncing
    uint32_t buttons = fd_ux_read_buttons(ux_button);
    fd_ux_button_timeout_logic(ux_button, buttons, fd_timer_get_timestamp(), fd_ux_button_send);
}

void fd_ux_button_change_logic(fd_ux_button_t *ux_button, uint32_t buttons, float timestamp) {
    fd_ux_button_check(ux_button, buttons, timestamp);
    ux_button->buttons = buttons;
}

void fd_ux_button_change(void *context) {
    fd_ux_button_t *ux_button = context;

    // ignore changes during debounce time
    if (ux_button->timer.active) {
        return;
    }

    uint32_t buttons = fd_ux_read_buttons(ux_button);
    fd_ux_button_change_logic(ux_button, buttons, fd_timer_get_timestamp());

    // start debounce timer
    fd_timer_start(&ux_button->timer, 0.05f);
}

void fd_ux_button_initialize(fd_ux_button_t *ux_button, const fd_ux_button_configuration_t *configuration) {
    fd_assert(fd_ux_button_manager.ux_button_count < (sizeof(fd_ux_button_manager.ux_buttons) / sizeof(fd_ux_button_manager.ux_buttons[0])));
    fd_ux_button_manager.ux_buttons[fd_ux_button_manager.ux_button_count++] = ux_button;

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

#if 0

typedef struct {
    fd_ux_button_t ux_button;
    fd_button_event_t events[256];
    uint8_t event_index;
} fd_ux_button_test_t;

fd_ux_button_test_t fd_ux_button_test;

void fd_ux_button_test_send(fd_ux_button_t *ux_button, fd_button_action_t action, uint32_t buttons, uint32_t holds, uint32_t chords, float timestamp, float duration) {
    fd_button_event_t event = {
        .action = action,
        .buttons = buttons,
        .holds = holds,
        .chords = chords,
        .timestamp = timestamp,
        .duration = duration,
    };
    fd_ux_button_test.events[fd_ux_button_test.event_index++] = event;
}

void fd_ux_button_test_reset(void) {
    memset(&fd_ux_button_test, 0, sizeof(fd_ux_button_test));
}

void fd_ux_button_test_1(void) {
    fd_ux_button_test_reset();

    // fast button press/release within debounce time
    float timestamp = 0.5f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b1, timestamp); // press
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b0, timestamp, fd_ux_button_test_send); // released before timeout
    fd_assert(fd_ux_button_test.event_index == 2);
    fd_assert(fd_ux_button_test.events[0].action == fd_button_action_pressed);
    fd_assert(fd_ux_button_test.events[1].action == fd_button_action_released);
}

void fd_ux_button_test_2(void) {
    fd_ux_button_test_reset();

    // normal button press, press debounce, release, release debounce
    float timestamp = 0.5f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b1, timestamp); // press
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b1, timestamp, fd_ux_button_test_send); // still pressed at timeout
    fd_assert(fd_ux_button_test.event_index == 1);
    fd_assert(fd_ux_button_test.events[0].action == fd_button_action_pressed);
    timestamp += 0.05f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b0, timestamp); // release
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b0, timestamp, fd_ux_button_test_send); // still released at timeout
    fd_assert(fd_ux_button_test.event_index == 2);
    fd_assert(fd_ux_button_test.events[1].action == fd_button_action_released);
}

void fd_ux_button_test_3(void) {
    fd_ux_button_test_reset();

    // normal button press, press debounce, release, press, debounce
    float timestamp = 0.5f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b1, timestamp); // press
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b1, timestamp, fd_ux_button_test_send); // still pressed at timeout
    fd_assert(fd_ux_button_test.event_index == 1);
    fd_assert(fd_ux_button_test.events[0].action == fd_button_action_pressed);
    timestamp += 0.05f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b0, timestamp); // release
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b1, timestamp, fd_ux_button_test_send); // pressed at timeout
    fd_assert(fd_ux_button_test.event_index == 1);
    timestamp += 0.05f;
    fd_ux_button_change_logic(&fd_ux_button_test.ux_button, 0b0, timestamp); // release
    timestamp += 0.05f;
    fd_ux_button_timeout_logic(&fd_ux_button_test.ux_button, 0b0, timestamp, fd_ux_button_test_send); // still released at timeout
    fd_assert(fd_ux_button_test.event_index == 2);
    fd_assert(fd_ux_button_test.events[3].action == fd_button_action_released);
}

void fd_ux_button_test_all(void) {
    fd_ux_button_test_1();
    fd_ux_button_test_2();
    fd_ux_button_test_3();
}

#endif

fd_source_pop()
