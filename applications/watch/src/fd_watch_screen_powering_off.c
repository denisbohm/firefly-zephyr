#include "fd_watch_screen_powering_off.h"

#include "fd_watch.h"

#include "fd_drawing_image.h"
#include "fd_drawing_text.h"
#include "fd_font_b612_regular_6.h"

#include "fd_assert.h"
#include "fd_unused.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_drawing_text_t text;
    fd_drawing_text_t countdown;
    fd_drawing_t drawings[2];
    fd_drawing_plane_t plane;
    
    char countdown_buffer[16];
    uint32_t ticks;
} fd_watch_screen_powering_off_t;

fd_watch_screen_powering_off_t fd_watch_screen_powering_off;

void fd_watch_screen_powering_off_add_drawing(void *object, fd_drawing_class_t *class) {
    const size_t size = sizeof(fd_watch_screen_powering_off.drawings) /  sizeof(fd_watch_screen_powering_off.drawings[0]);
    if (fd_watch_screen_powering_off.plane.drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    fd_watch_screen_powering_off.drawings[fd_watch_screen_powering_off.plane.drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++fd_watch_screen_powering_off.plane.drawing_count;
}

bool fd_watch_screen_powering_off_animate(void) {
    if (fd_watch_screen_powering_off.ticks++ < (5 * 50)) {
        uint32_t countdown = 5 - fd_watch_screen_powering_off.ticks / 50;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%u", countdown);
        fd_view_string_update(&fd_watch_screen_powering_off.countdown.view.string, buffer);
    } else {
        fd_watch_power_off();
    }

    return false;
}

void fd_watch_screen_powering_off_preview(void) {
}

void fd_watch_screen_powering_off_activate(void) {
    fd_watch_screen_powering_off.ticks = 0;
}

void fd_watch_screen_powering_off_deactivate(void) {
}

bool fd_watch_screen_powering_off_button(const fd_ux_button_event_t *event) {
    if (fd_ux_button_is_any_activated(event, fd_watch_button_both)) {
        fd_ux_set_screen(fd_watch_screen_id_time);
    }
    return false;
}

void fd_watch_screen_powering_off_initialize(fd_ux_screen_t *screen) {
    memset(&fd_watch_screen_powering_off, 0, sizeof(fd_watch_screen_powering_off));
    
    fd_drawing_plane_t *plane = &fd_watch_screen_powering_off.plane;
    plane->drawings = fd_watch_screen_powering_off.drawings;
    screen->planes[0] = plane;
    screen->plane_count = 1;
    screen->preview = fd_watch_screen_powering_off_preview;
    screen->activate = fd_watch_screen_powering_off_activate;
    screen->deactivate = fd_watch_screen_powering_off_deactivate;
    screen->button = fd_watch_screen_powering_off_button;
    screen->animate = fd_watch_screen_powering_off_animate;
    
    fd_watch_screen_powering_off.text.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 64, .y = 64 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = "Powering Off In:", .length = 16 },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_watch_screen_powering_off_add_drawing(&fd_watch_screen_powering_off.text, &fd_drawing_text_class);
    
    strlcpy(fd_watch_screen_powering_off.countdown_buffer, "5", sizeof(fd_watch_screen_powering_off.countdown_buffer));
    fd_watch_screen_powering_off.countdown.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 64, .y = 96 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = fd_watch_screen_powering_off.countdown_buffer, .length = 1 },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_watch_screen_powering_off_add_drawing(&fd_watch_screen_powering_off.countdown, &fd_drawing_text_class);
}
