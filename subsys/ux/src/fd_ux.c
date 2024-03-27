#include "fd_ux.h"

#include "fd_assert.h"
#include "fd_canvas.h"
#if CONFIG_FIREFLY_SUBSYS_STORAGE_KEY_VALUE_STORE
#include "fd_key_value_store.h"
#endif

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_ux_t *uxs[4];
    uint32_t ux_count;
} fd_ux_manager_t;

fd_ux_manager_t fd_ux_manager;

fd_ux_t *fd_ux_get(uint32_t identifier) {
    for (uint32_t i = 0; i < fd_ux_manager.ux_count; ++i) {
        fd_ux_t *ux = fd_ux_manager.uxs[i];
        if (ux->configuration.identifier == identifier) {
            return ux;
        }
    }
    return NULL;
}

void fd_ux_display_configuration_default(fd_ux_t *ux) {
    ux->display_configuration = (firefly_ux_v1_DisplayConfiguration) {
        .timeout = 0.0,
    };
}

void fd_ux_interaction_configuration_default(fd_ux_t *ux) {
    memset(&ux->interaction_configuration, 0, sizeof(ux->interaction_configuration));
}

#if CONFIG_FIREFLY_SUBSYS_STORAGE_KEY_VALUE_STORE

void fd_ux_persist_get_display_configuration(fd_ux_t *ux) {
    bool result = fd_key_value_store_get(ux->display_configuration_key, &firefly_ux_v1_DisplayConfiguration_msg, &ux->display_configuration);
    if (!result) {
        fd_ux_display_configuration_default(ux);
    }
    ux->configuration.idle_ticks = ux->display_configuration.timeout * 50;
}

void fd_ux_persist_get_interaction_configuration(fd_ux_t *ux) {
    bool result = fd_key_value_store_get(ux->interaction_configuration_key, &firefly_ux_v1_InteractionConfiguration_msg, &ux->interaction_configuration);
    if (!result) {
        fd_ux_interaction_configuration_default(ux);
    }
}

void fd_ux_persist_was_changed(void *context, const char *key) {
    fd_ux_t *ux = context;
    if (strcmp(key, ux->display_configuration_key) == 0) {
        fd_ux_persist_get_display_configuration(ux);
    } else
    if (strcmp(key, ux->interaction_configuration_key) == 0) {
        fd_ux_persist_get_interaction_configuration(ux);
    }
}

void fd_ux_persist_get(void *context) {
    fd_ux_t *ux = context;
    fd_ux_persist_get_display_configuration(ux);
    fd_ux_persist_get_interaction_configuration(ux);
}

void fd_ux_get_display_configuration(fd_ux_t *ux, firefly_ux_v1_DisplayConfiguration *display_configuration) {
    memcpy(display_configuration, &ux->display_configuration, sizeof(ux->display_configuration));
}

void fd_ux_set_display_configuration(fd_ux_t *ux, const firefly_ux_v1_DisplayConfiguration *display_configuration) {
    bool result = fd_key_value_store_set(ux->display_configuration_key, &firefly_ux_v1_DisplayConfiguration_msg, display_configuration);
    fd_assert(result);
    fd_ux_active(ux);
}

void fd_ux_get_interaction_configuration(fd_ux_t *ux, firefly_ux_v1_InteractionConfiguration *interaction_configuration) {
    memcpy(interaction_configuration, &ux->interaction_configuration, sizeof(ux->interaction_configuration));
}

void fd_ux_set_interaction_configuration(fd_ux_t *ux, const firefly_ux_v1_InteractionConfiguration *interaction_configuration) {
    bool result = fd_key_value_store_set(ux->interaction_configuration_key, &firefly_ux_v1_InteractionConfiguration_msg, interaction_configuration);
    fd_assert(result);
}

#endif

void fd_ux_update(fd_ux_t *ux) {
    fd_canvas_t *canvas = &ux->canvas;
    fd_canvas_update(canvas);
    fd_canvas_render(canvas);
}

bool fd_ux_get_update_enabled(fd_ux_t *ux) {
    return !ux->update_enabled;
}

void fd_ux_set_update_enabled(fd_ux_t *ux, bool update_enabled) {
    ux->update_enabled = update_enabled;
}

void fd_ux_active(fd_ux_t *ux) {
    switch (ux->state) {
        case fd_ux_state_on:
            ux->idle_ticks = 0;
        break;
        case fd_ux_state_idle:
            fd_ux_set_state(ux, fd_ux_state_on);
        break;
        case fd_ux_state_off:
        break;
    }
}

void fd_ux_set_idle_ticks(fd_ux_t *ux, uint32_t ticks) {
    ux->configuration.idle_ticks = ticks;
    ux->idle_ticks = 0;
}

fd_ux_state_t fd_ux_get_state(fd_ux_t *ux) {
    return ux->state;
}

void fd_ux_set_state(fd_ux_t *ux, fd_ux_state_t state) {
    if (state == ux->state) {
        return;
    }

    fd_ux_state_t old_state = ux->state;
    ux->idle_ticks = 0;
    ux->state = state;

    if (state == fd_ux_state_on) {
        if (ux->screen->animate != NULL) {
            ux->screen->animate();
        }
        fd_ux_update(ux);
    }

    if (ux->configuration.state_changed != NULL) {
        ux->configuration.state_changed(old_state, state);
    }
    for (uint32_t i = 0; i < ux->listener_count; ++i) {
        const fd_ux_listener_t *listener = ux->listeners[i];
        if (listener->state_changed != NULL) {
            listener->state_changed(listener->context, old_state, state);
        }
    }
}

bool fd_ux_should_idle(fd_ux_t *ux) {
    return (ux->state == fd_ux_state_on) && (ux->configuration.idle_ticks != 0) && (ux->idle_ticks > ux->configuration.idle_ticks);
}

void fd_ux_tick(void) {
    for (uint32_t i = 0; i < fd_ux_manager.ux_count; ++i) {
        fd_ux_t *ux = fd_ux_manager.uxs[i];
        if (ux->configuration.idle_ticks != 0) {
            if (ux->state == fd_ux_state_on) {
                ++ux->idle_ticks;
                if (fd_ux_should_idle(ux)) {
                    fd_ux_set_state(ux, fd_ux_state_idle);
                    return;
                }
            }
        }

        if (ux->update_enabled) {
            if (ux->screen->animate) {
                ux->screen->animate();
            }
            if (ux->state == fd_ux_state_on) {
                fd_ux_update(ux);
            }
        }
    }
}

fd_ux_screen_t *fd_ux_get_active_screen(fd_ux_t *ux) {
    return ux->screen;
}

void fd_ux_add_listener(fd_ux_t *ux, const fd_ux_listener_t *listener) {
    fd_assert(ux->listener_count < (sizeof(ux->listeners) / sizeof(ux->listeners[0])));
    ux->listeners[ux->listener_count++] = listener;
}

void fd_ux_initialize(fd_ux_t *ux, const fd_ux_configuration_t *configuration) {
    fd_assert(fd_ux_manager.ux_count < (sizeof(fd_ux_manager.uxs) / sizeof(fd_ux_manager.uxs[0])));
    fd_ux_manager.uxs[fd_ux_manager.ux_count++] = ux;

    memset(ux, 0, sizeof(*ux));
    ux->configuration = *configuration;

    snprintf(ux->display_configuration_key, sizeof(ux->display_configuration_key), "ux/%u/display", configuration->identifier);
    snprintf(ux->interaction_configuration_key, sizeof(ux->interaction_configuration_key), "ux/%u/interaction", configuration->identifier);
    
    for (int id = 0; id < ux->configuration.screen_count; ++id) {
        ux->configuration.screens[id].id = id;
    }
    
    for (int id = 0; id < ux->configuration.screen_count; ++id) {
        fd_assert(ux->configuration.screens[id].plane_count > 0);
    }

    fd_canvas_initialize(&ux->canvas);
    ux->canvas.graphics = configuration->graphics;

    ux->animation = true;
    ux->update_enabled = true;
    fd_ux_set_screen(ux, ux->configuration.initial_screen);

#if CONFIG_FIREFLY_SUBSYS_STORAGE_KEY_VALUE_STORE
    fd_key_value_store_listener_t listener = {
        .was_set = fd_ux_persist_was_changed,
        .was_removed = fd_ux_persist_was_changed,
        .was_erased = fd_ux_persist_get,
    };
    fd_key_value_store_add_listener(&listener);
    fd_ux_persist_get(ux);
#endif
}

static void fd_ux_set_screen_to(fd_ux_t *ux, uint32_t screen_id, bool preview) {
    fd_ux_screen_t *screen = &ux->configuration.screens[screen_id];
    if (ux->screen == screen) {
        return;
    }

    if (ux->screen) {
        ux->screen->deactivate();
    }
    
    ux->screen = screen;
    
    fd_canvas_t *canvas = &ux->canvas;
    canvas->plane_count = 0;
    for (uint32_t i = 0; i < screen->plane_count; ++i) {
        canvas->planes[canvas->plane_count] = screen->planes[i];
        ++canvas->plane_count;
    }
    if (ux->configuration.plane != NULL) {
        canvas->planes[canvas->plane_count] = ux->configuration.plane;
        ++canvas->plane_count;
    }
    
    if (preview) {
        screen->preview();
    } else {
        screen->activate();
    }
    
    fd_canvas_update(canvas);
    int width = canvas->graphics->width;
    int height = canvas->graphics->height;
    canvas->change.area = (fd_graphics_area_t) { .x = 0, .y = 0, .width = width, .height = height };
    canvas->change.opaque = false;
    fd_canvas_render(canvas);
}

uint32_t fd_ux_get_screen(fd_ux_t *ux) {
    return ux->screen != NULL ? ux->screen->id : 0;
}

void fd_ux_set_screen(fd_ux_t *ux, uint32_t screen_id) {
    fd_ux_set_screen_to(ux, screen_id, false);
}
;
void fd_ux_set_animation(fd_ux_t *ux, bool animation) {
    ux->animation = animation;
}

void fd_ux_set_screen_preview(fd_ux_t *ux, uint32_t screen_id) {
    fd_ux_set_screen_to(ux, screen_id, true);
}

fd_ux_frame_buffer_t fd_ux_get_frame_buffer(fd_ux_t *ux) {
    fd_graphics_t *graphics = ux->configuration.graphics;
    return (fd_ux_frame_buffer_t) {
        .data = graphics->buffer,
        .size = graphics->width * graphics->height * 3,
    };
}

fd_ux_frame_buffer_metadata_t fd_ux_get_frame_buffer_metadata(fd_ux_t *ux) {
    fd_graphics_t *graphics = ux->configuration.graphics;
    return (fd_ux_frame_buffer_metadata_t) {
        .width = graphics->width,
        .height = graphics->height,
        .type = fd_ux_frame_buffer_type_color,
        .components = {
            .color = {
                .r = 6,
                .g = 6,
                .b = 6,
            }
        }
    };
}
