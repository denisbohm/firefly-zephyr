#include "fd_ux_watch.h"

#include "fd_assert.h"
#include "fd_calendar.h"
#include "fd_drawing_text.h"
#include "fd_font_b612_regular_6.h"
#include "fd_unused.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_drawing_text_t text;
    fd_drawing_t drawings[1];
    fd_drawing_plane_t plane;
    char time_string[16];
} fd_ux_watch_t;

fd_ux_watch_t fd_ux_watch;

void fd_ux_watch_add_drawing(void *object, fd_drawing_class_t *class) {
    const size_t size = sizeof(fd_ux_watch.drawings) /  sizeof(fd_ux_watch.drawings[0]);
    if (fd_ux_watch.plane.drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    fd_ux_watch.drawings[fd_ux_watch.plane.drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++fd_ux_watch.plane.drawing_count;
}

int64_t fd_rtc_get_utc(void);

#define UTC_2001 978307200

void fd_ux_watch_update(void) {
    // preferences
    int32_t utc_offset = -5 * 60 * 60;
    bool is_12_hour = true;

    int64_t utc = fd_rtc_get_utc();
    fd_calendar_t calendar = fd_calendar_from_time((utc < UTC_2001) ? utc : (utc + utc_offset));
    fd_calendar_12_hour_t clock_12_hour = fd_calendar_get_12_hour(&calendar);
    char buffer[16];
    if (is_12_hour) {
        snprintf(buffer, sizeof(buffer), "%u:%02u:%02u %cm", clock_12_hour.hour, calendar.min, calendar.sec, clock_12_hour.meridiem);
    } else {
        snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", calendar.hour, calendar.min, calendar.sec);
    }
    fd_view_string_update(&fd_ux_watch.text.view.string, buffer);
}

bool fd_ux_watch_animate(void) {
    fd_ux_watch_update();
    return false;
}

void fd_ux_watch_preview(void) {
}

void fd_ux_watch_activate(void) {
}

void fd_ux_watch_deactivate(void) {
}

bool fd_ux_watch_button(fd_button_event_t event fd_unused) {
    fd_ux_set_screen(fd_ux_screen_id_splash);
    return false;
}

void fd_ux_watch_initialize(fd_ux_screen_t *screen) {
    memset(&fd_ux_watch, 0, sizeof(fd_ux_watch));
    
    fd_drawing_plane_t *plane = &fd_ux_watch.plane;
    plane->drawings = fd_ux_watch.drawings;
    screen->planes[0] = plane;
    screen->plane_count = 1;
    screen->preview = fd_ux_watch_preview;
    screen->activate = fd_ux_watch_activate;
    screen->deactivate = fd_ux_watch_deactivate;
    screen->button = fd_ux_watch_button;
    screen->animate = fd_ux_watch_animate;
    
    fd_ux_watch.text.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 64, .y = 64 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = fd_ux_watch.time_string, .size = sizeof(fd_ux_watch.time_string) },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_ux_watch_add_drawing(&fd_ux_watch.text, &fd_drawing_text_class);
    
    fd_ux_watch_update();
}
