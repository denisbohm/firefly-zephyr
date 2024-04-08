#include "fd_ux_touch.h"

#include "fd_assert.h"
#include "fd_ux.h"

#include <string.h>

typedef struct {
    fd_ux_touch_t *ux_touchs[4];
    uint32_t ux_touch_count;
} fd_ux_touch_manager_t;

fd_ux_touch_manager_t fd_ux_touch_manager;

fd_ux_touch_t *fd_ux_touch_get(uint32_t identifier) {
    for (uint32_t i = 0; i < fd_ux_touch_manager.ux_touch_count; ++i) {
        fd_ux_touch_t *ux_touch = fd_ux_touch_manager.ux_touchs[i];
        if (ux_touch->configuration.identifier == identifier) {
            return ux_touch;
        }
    }
    return NULL;
}

bool fd_ux_touch_is_any_pressed(fd_ux_touch_t *ux_touch) {
    return ux_touch->touchs != 0;
}

void fd_ux_touch_ux_state_changed(void *context, fd_ux_state_t old, fd_ux_state_t new) {
    fd_ux_touch_t *ux_touch = context;
    ux_touch->consume_release = fd_ux_touch_is_any_pressed(ux_touch);
}

void fd_ux_touch_event(fd_ux_touch_t *ux_touch, const fd_touch_event_t *event) {
    if (event->action == fd_touch_action_pressed) {
        ux_touch->touchs = 1;
    } else {
        ux_touch->touchs = 0;
    }
    if (ux_touch->consume_release) {
        if (ux_touch->touchs == 0) {
            ux_touch->consume_release = false;
        }
        return;
    }

    fd_ux_t *ux = ux_touch->configuration.ux;
    if (fd_ux_get_state(ux) != fd_ux_state_on) {
        fd_ux_set_state(ux, fd_ux_state_on);
        return;
    }

    fd_ux_active(ux);
    fd_ux_screen_t *screen = fd_ux_get_active_screen(ux);
    if (screen->touch != NULL) {
        screen->touch(event);
    }
}

void fd_ux_touch_send(fd_ux_touch_t *ux_touch, fd_touch_action_t action, fd_touch_gesture_t gesture, int32_t x, int32_t y) {
    fd_touch_event_t event = {
        .action = action,
        .gesture = gesture,
        .x = x,
        .y = y,
    };

    if (ux_touch->configuration.callback != NULL) {
        ux_touch->configuration.callback(ux_touch, &event);
    } else {
        fd_ux_touch_event(ux_touch, &event);
    }
}

void fd_ux_touch_initialize(fd_ux_touch_t *ux_touch, const fd_ux_touch_configuration_t *configuration) {
    fd_assert(fd_ux_touch_manager.ux_touch_count < (sizeof(fd_ux_touch_manager.ux_touchs) / sizeof(fd_ux_touch_manager.ux_touchs[0])));
    fd_ux_touch_manager.ux_touchs[fd_ux_touch_manager.ux_touch_count++] = ux_touch;

    memset(ux_touch, 0, sizeof(*ux_touch));
    ux_touch->configuration = *configuration;
    
    ux_touch->listener = (fd_ux_listener_t) {
        .context = ux_touch,
        .state_changed = fd_ux_touch_ux_state_changed,
    };
    fd_ux_add_listener(ux_touch->configuration.ux, &ux_touch->listener);
}
