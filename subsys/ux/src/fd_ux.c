#include "fd_ux.h"

#include "fd_assert.h"
#include "fd_canvas.h"
#include "fd_event.h"
#include "fd_gpio.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char *tick_event_name;
} fd_ux_static_t;

static const fd_ux_static_t fd_ux_static = {
    .tick_event_name = "fd_ux.tick",
};

typedef struct {
    fd_ux_configuration_t *configuration;
    
    fd_canvas_t canvas;
    fd_ux_screen_t *screen;

    bool is_off;
    uint32_t tick_event;
    
    volatile bool animation;
} fd_ux_t;

fd_ux_t fd_ux;

bool fd_ux_is_powered_on(void) {
    return !fd_ux.is_off;
}

void fd_ux_power_off(void) {
    fd_ux.is_off = true;
}

void fd_ux_power_on(void) {
    fd_ux.is_off = false;
}

void fd_ux_update(void) {
    if (fd_ux.screen->animate) {
        fd_ux.screen->animate();
    }
    
    fd_canvas_t *canvas = &fd_ux.canvas;
    fd_canvas_update(canvas);

    fd_canvas_render(canvas);
}

void fd_ux_button_event(const fd_ux_button_event_t *event) {
    fd_ux.screen->button(event);
}

void fd_ux_tick_event(uint32_t identifier) {
    if (!fd_ux.is_off) {
        fd_ux_update();
    }
}

void fd_ux_initialize(fd_ux_configuration_t *configuration) {
    memset(&fd_ux, 0, sizeof(fd_ux));
    fd_ux.configuration = configuration;
    
    fd_ux.tick_event = fd_event_get_identifier(fd_ux_static.tick_event_name);
    fd_event_add_callback(fd_ux.tick_event, fd_ux_tick_event);

    for (int id = 0; id < fd_ux.configuration->screen_count; ++id) {
        fd_ux.configuration->screens[id].id = id;
    }
    
    for (int id = 0; id < fd_ux.configuration->screen_count; ++id) {
        fd_assert(fd_ux.configuration->screens[id].plane_count > 0);
    }

    fd_canvas_initialize(&fd_ux.canvas);
    fd_ux.canvas.graphics = configuration->graphics;
    
    fd_ux_set_screen(0);

    fd_ux.animation = true;
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
