#include "fd_ux.h"

#include "fd_assert.h"
#include "fd_canvas.h"
#include "fd_event.h"
#include "fd_gpio.h"
#include "fd_graphics.h"
#include "fd_graphics_ssd1327.h"
#include "fd_ssd1327.h"
#include "fd_ux_splash.h"
#include "fd_ux_watch.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_gpio_t button_a_gpio;
    fd_gpio_t button_b_gpio;
    char *tick_event_name;
    char *button_event_name;
} fd_ux_static_t;

static const fd_ux_static_t fd_ux_static = {
    .button_a_gpio = { .port = 0, .pin = 9 },
    .button_b_gpio = { .port = 1, .pin = 10 },
    .tick_event_name = "fd_ux.tick",
    .button_event_name = "fd_ux.button",
};

typedef struct {
    fd_canvas_t canvas;
    
    fd_ux_screen_t screens[fd_ux_screen_id_count];
    fd_ux_screen_t *screen;
    
    volatile bool animation;

    bool is_off;

    uint32_t tick_event;
    uint32_t button_event;
    unsigned button_state;
} fd_ux_t;

fd_ux_t fd_ux;

void fd_ux_power_off(void) {
    fd_ssd1327_display_off();

    fd_ux.is_off = true;
}

void fd_ux_power_on(void) {
    fd_ux.is_off = false;

    fd_ssd1327_display_on();
}

void fd_ux_update(void) {
    if (fd_ux.screen->animate) {
        fd_ux.screen->animate();
    }
    
    fd_canvas_t *canvas = &fd_ux.canvas;
    fd_canvas_update(canvas);

    fd_canvas_render(canvas);
}

void fd_ux_button_changed(void) {
    int old_state = fd_ux.button_state;
    bool a = fd_gpio_get(fd_ux_static.button_a_gpio);
    bool b = fd_gpio_get(fd_ux_static.button_b_gpio);
    int new_state = (a ? 0b10 : 0b00) | (b ? 0b01 : 0b00);
    if (new_state == old_state) {
        return;
    }
    fd_ux.button_state = new_state;
    
    if ((old_state == 0b00) && (new_state == 0b10)) {
        fd_event_set_from_interrupt(fd_ux.button_event);
    }
}

void fd_ux_button_event(uint32_t identifier) {
    fd_button_event_t event = {};
    fd_ux.screen->button(event);
}

void fd_ux_tick_event(uint32_t identifier) {
    fd_ux_update();
}

void fd_ux_initialize(void) {
    memset(&fd_ux, 0, sizeof(fd_ux));

    fd_ux.button_event = fd_event_get_identifier(fd_ux_static.button_event_name);
    fd_event_add_callback(fd_ux.button_event, fd_ux_button_event);

    fd_ux.tick_event = fd_event_get_identifier(fd_ux_static.tick_event_name);
    fd_event_add_callback(fd_ux.tick_event, fd_ux_tick_event);

    memset(&fd_ux, 0, sizeof(fd_ux));
    for (int id = 0; id < fd_ux_screen_id_count; ++id) {
        fd_ux.screens[id].id = id;
    }
    
    fd_ux_splash_initialize(&fd_ux.screens[fd_ux_screen_id_splash]);
    fd_ux_watch_initialize(&fd_ux.screens[fd_ux_screen_id_watch]);

    for (int id = 0; id < fd_ux_screen_id_count; ++id) {
        fd_assert(fd_ux.screens[id].plane_count > 0);
    }

    fd_canvas_initialize(&fd_ux.canvas);
    fd_ux.canvas.graphics = fd_graphics_ssd1327_get();
    
    fd_ux_set_screen(fd_ux_screen_id_splash);

    fd_ux.animation = true;

    fd_gpio_set_callback(fd_ux_static.button_a_gpio, fd_ux_button_changed);
    fd_gpio_set_callback(fd_ux_static.button_b_gpio, fd_ux_button_changed);
}

static void fd_ux_set_screen_to(fd_ux_screen_id_t id, bool preview) {
    if (fd_ux.screen) {
        fd_ux.screen->deactivate();
    }
    
    fd_ux_screen_t *screen = &fd_ux.screens[id];
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
    canvas->change.area = (fd_graphics_area_t) { .x = 0, .y = 0, .width = 128, .height = 128 };
    canvas->change.opaque = false;
    fd_canvas_render(canvas);
}

void fd_ux_set_screen(fd_ux_screen_id_t id) {
    fd_ux_set_screen_to(id, false);
}

void fd_ux_set_animation(bool animation) {
    fd_ux.animation = animation;
}

void fd_ux_set_screen_preview(fd_ux_screen_id_t id) {
    fd_ux_set_screen_to(id, true);
}
