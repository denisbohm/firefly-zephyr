#include "fd_watch_screen_splash.h"

#include "fd_watch.h"

#include "fd_drawing_image.h"
#include "fd_drawing_text.h"
#include "fd_font_b612_regular_6.h"

#include "fd_assert.h"
#include "fd_unused.h"

#include <string.h>

typedef struct {
    fd_drawing_text_t text;
    fd_drawing_image_t image;
    fd_drawing_t drawings[2];
    fd_drawing_plane_t plane;
    
    uint32_t ticks;
} fd_watch_screen_splash_t;

fd_watch_screen_splash_t fd_watch_screen_splash;

void fd_watch_screen_splash_add_drawing(void *object, fd_drawing_class_t *class) {
    const size_t size = sizeof(fd_watch_screen_splash.drawings) /  sizeof(fd_watch_screen_splash.drawings[0]);
    if (fd_watch_screen_splash.plane.drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    fd_watch_screen_splash.drawings[fd_watch_screen_splash.plane.drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++fd_watch_screen_splash.plane.drawing_count;
}

bool fd_watch_screen_splash_animate(void) {
    if (fd_watch_screen_splash.ticks++ > 2 * 50) {
        fd_ux_t *ux = fd_ux_get(0);
        fd_ux_set_screen(ux, fd_watch_screen_id_time);
    }
    return false;
}

void fd_watch_screen_splash_preview(void) {
}

void fd_watch_screen_splash_activate(void) {
    fd_watch_screen_splash.ticks = 0;
}

void fd_watch_screen_splash_deactivate(void) {
}

bool fd_watch_screen_splash_button(const fd_button_event_t *event) {
    if (fd_button_was_pressed(event, fd_watch_button_both)) {
        fd_ux_t *ux = fd_ux_get(0);
        fd_ux_set_screen(ux, fd_watch_screen_id_time);
    }
    return false;
}

void fd_watch_screen_splash_initialize(fd_ux_screen_t *screen) {
    memset(&fd_watch_screen_splash, 0, sizeof(fd_watch_screen_splash));
    
    fd_drawing_plane_t *plane = &fd_watch_screen_splash.plane;
    plane->drawings = fd_watch_screen_splash.drawings;
    screen->planes[0] = plane;
    screen->plane_count = 1;
    screen->preview = fd_watch_screen_splash_preview;
    screen->activate = fd_watch_screen_splash_activate;
    screen->deactivate = fd_watch_screen_splash_deactivate;
    screen->button = fd_watch_screen_splash_button;
    screen->animate = fd_watch_screen_splash_animate;
    
    fd_watch_screen_splash.text.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 48, .y = 32 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = "Hello", .length = 5 },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_watch_screen_splash_add_drawing(&fd_watch_screen_splash.text, &fd_drawing_text_class);
    
#if 0
    fd_watch_screen_splash.image.view = (fd_view_image_t) {
        .visible = true,
        .location = { .x = 64, .y = 96 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .image = { .image = &fd_image_splash },
    };
    fd_watch_screen_splash_add_drawing(&fd_watch_screen_splash.image, &fd_drawing_image_class);
#endif
}
