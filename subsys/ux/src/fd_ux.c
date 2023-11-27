#include "fd_ux.h"

#include "fd_assert.h"
#include "fd_canvas.h"
#include "fd_gpio.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_ux_configuration_t *configuration;
    
    fd_canvas_t canvas;
    fd_ux_screen_t *screen;

    bool update_enabled;
    uint32_t tick_event;
    uint32_t idle_ticks;
    
    bool animation;
} fd_ux_t;

fd_ux_t fd_ux;

void fd_ux_update(void) {
    if (fd_ux.screen->animate) {
        fd_ux.screen->animate();
    }
    
    fd_canvas_t *canvas = &fd_ux.canvas;
    fd_canvas_update(canvas);

    fd_canvas_render(canvas);
}

bool fd_ux_get_update_enabled(void) {
    return !fd_ux.update_enabled;
}

void fd_ux_set_update_enabled(bool update_enabled) {
    fd_ux.update_enabled = update_enabled;
}

bool fd_ux_get_idle(void) {
    return (fd_ux.configuration->idle_ticks != 0) && (fd_ux.idle_ticks > fd_ux.configuration->idle_ticks);
}

void fd_ux_set_idle(bool idle) {
    if (idle) {
        fd_ux.idle_ticks = fd_ux.configuration->idle_ticks + 1;
    } else {
        fd_ux.idle_ticks = 0;
    }
}

void fd_ux_button_event(const fd_ux_button_event_t *event) {
    bool idle = fd_ux_get_idle();
    fd_ux.idle_ticks = 0;
    if (idle) {
        if (fd_ux.configuration->idle) {
            fd_ux.configuration->idle(false);
        }
    }

    fd_ux.screen->button(event);
}

void fd_ux_tick(void) {
    if (fd_ux.configuration->idle_ticks != 0) {
        if (!fd_ux_get_idle()) {
            ++fd_ux.idle_ticks;
            if (fd_ux.idle_ticks > fd_ux.configuration->idle_ticks) {
                if (fd_ux.configuration->idle) {
                    fd_ux.configuration->idle(true);
                }
            }
        }
    }

    if (fd_ux.update_enabled) {
        fd_ux_update();
    }
}

void fd_ux_initialize(fd_ux_configuration_t *configuration) {
    memset(&fd_ux, 0, sizeof(fd_ux));
    fd_ux.configuration = configuration;
    
    for (int id = 0; id < fd_ux.configuration->screen_count; ++id) {
        fd_ux.configuration->screens[id].id = id;
    }
    
    for (int id = 0; id < fd_ux.configuration->screen_count; ++id) {
        fd_assert(fd_ux.configuration->screens[id].plane_count > 0);
    }

    fd_canvas_initialize(&fd_ux.canvas);
    fd_ux.canvas.graphics = configuration->graphics;

    fd_ux.animation = true;
    fd_ux.update_enabled = true;
    fd_ux_set_screen(fd_ux.configuration->initial_screen);
}

static void fd_ux_set_screen_to(uint32_t screen_id, bool preview) {
    if (fd_ux.screen) {
        fd_ux.screen->deactivate();
    }
    
    fd_ux_screen_t *screen = &fd_ux.configuration->screens[screen_id];
    fd_ux.screen = screen;
    
    fd_canvas_t *canvas = &fd_ux.canvas;
    uint32_t plane_count = screen->plane_count;
    for (uint32_t i = 0; i < plane_count; ++i) {
        canvas->planes[i] = screen->planes[i];
    }
    canvas->plane_count = plane_count;
    
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

uint32_t fd_ux_get_screen(void) {
    return fd_ux.screen != NULL ? fd_ux.screen->id : 0;
}

void fd_ux_set_screen(uint32_t screen_id) {
    fd_ux_set_screen_to(screen_id, false);
}
;
void fd_ux_set_animation(bool animation) {
    fd_ux.animation = animation;
}

void fd_ux_set_screen_preview(uint32_t screen_id) {
    fd_ux_set_screen_to(screen_id, true);
}
