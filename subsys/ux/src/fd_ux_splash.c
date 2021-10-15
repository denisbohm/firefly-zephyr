#include "fd_ux_splash.h"

#include "fd_assert.h"
#include "fd_drawing_image.h"
#include "fd_drawing_text.h"
#include "fd_font_b612_regular_6.h"
#include "fd_unused.h"

#include <string.h>

typedef struct {
    fd_drawing_text_t text;
    fd_drawing_image_t image;
    fd_drawing_t drawings[2];
    fd_drawing_plane_t plane;
    
    uint32_t ticks;
} fd_ux_splash_t;

fd_ux_splash_t fd_ux_splash;

void fd_ux_splash_add_drawing(void *object, fd_drawing_class_t *class) {
    const size_t size = sizeof(fd_ux_splash.drawings) /  sizeof(fd_ux_splash.drawings[0]);
    if (fd_ux_splash.plane.drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    fd_ux_splash.drawings[fd_ux_splash.plane.drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++fd_ux_splash.plane.drawing_count;
}

bool fd_ux_splash_animate(void) {
    if (fd_ux_splash.ticks++ > 2 * 50) {
        fd_ux_set_screen(fd_ux_screen_id_watch);
    }
    return false;
}

void fd_ux_splash_preview(void) {
}

void fd_ux_splash_activate(void) {
    fd_ux_splash.ticks = 0;
}

void fd_ux_splash_deactivate(void) {
}

bool fd_ux_splash_button(fd_button_event_t event fd_unused) {
    fd_ux_set_screen(fd_ux_screen_id_watch);
    return false;
}

void fd_ux_splash_initialize(fd_ux_screen_t *screen) {
    memset(&fd_ux_splash, 0, sizeof(fd_ux_splash));
    
    fd_drawing_plane_t *plane = &fd_ux_splash.plane;
    plane->drawings = fd_ux_splash.drawings;
    screen->planes[0] = plane;
    screen->plane_count = 1;
    screen->preview = fd_ux_splash_preview;
    screen->activate = fd_ux_splash_activate;
    screen->deactivate = fd_ux_splash_deactivate;
    screen->button = fd_ux_splash_button;
    screen->animate = fd_ux_splash_animate;
    
    fd_ux_splash.text.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 64, .y = 64 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = "Hello", .length = 5 },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_ux_splash_add_drawing(&fd_ux_splash.text, &fd_drawing_text_class);
    
#if 0
    fd_ux_splash.image.view = (fd_view_image_t) {
        .visible = true,
        .location = { .x = 64, .y = 96 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .image = { .image = &fd_image_splash },
    };
    fd_ux_splash_add_drawing(&fd_ux_splash.image, &fd_drawing_image_class);
#endif
}
