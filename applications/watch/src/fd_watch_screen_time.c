#include "fd_watch_screen_time.h"

#include "fd_watch.h"

#include "fd_drawing_text.h"
#include "fd_font_b612_regular_6.h"

#include "fd_assert.h"
#include "fd_calendar.h"
#include "fd_unused.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_drawing_text_t text;
    fd_drawing_t drawings[1];
    fd_drawing_plane_t plane;
    char time_string[16];
} fd_watch_screen_time_t;

fd_watch_screen_time_t fd_watch_screen_time;

void fd_watch_screen_time_add_drawing(void *object, fd_drawing_class_t *class) {
    const size_t size = sizeof(fd_watch_screen_time.drawings) /  sizeof(fd_watch_screen_time.drawings[0]);
    if (fd_watch_screen_time.plane.drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    fd_watch_screen_time.drawings[fd_watch_screen_time.plane.drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++fd_watch_screen_time.plane.drawing_count;
}

int64_t fd_rtc_get_utc(void);

#define UTC_2001 978307200

void fd_watch_screen_time_update(void) {
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
    fd_view_string_update(&fd_watch_screen_time.text.view.string, buffer);
}

bool fd_watch_screen_time_animate(void) {
    fd_watch_screen_time_update();
    return false;
}

void fd_watch_screen_time_preview(void) {
}

void fd_watch_screen_time_activate(void) {
}

void fd_watch_screen_time_deactivate(void) {
}

bool fd_watch_screen_time_button(const fd_ux_button_event_t *event) {
    if (fd_ux_button_is_one_activated(event, fd_watch_button_left)) {
        fd_ux_set_screen(fd_watch_screen_id_splash);
    }
    return false;
}

void fd_watch_screen_time_initialize(fd_ux_screen_t *screen) {
    memset(&fd_watch_screen_time, 0, sizeof(fd_watch_screen_time));
    
    fd_drawing_plane_t *plane = &fd_watch_screen_time.plane;
    plane->drawings = fd_watch_screen_time.drawings;
    screen->planes[0] = plane;
    screen->plane_count = 1;
    screen->preview = fd_watch_screen_time_preview;
    screen->activate = fd_watch_screen_time_activate;
    screen->deactivate = fd_watch_screen_time_deactivate;
    screen->button = fd_watch_screen_time_button;
    screen->animate = fd_watch_screen_time_animate;
    
    fd_watch_screen_time.text.view = (fd_view_text_t) {
        .visible = true,
        .location = { .x = 64, .y = 64 },
        .alignments = { .x = fd_view_alignment_center, .y = fd_view_alignment_center },
        .string = { .string = fd_watch_screen_time.time_string, .size = sizeof(fd_watch_screen_time.time_string) },
        .font.font = &fd_font_b612_regular_6,
        .color = { .argb = 0xffffffff },
    };
    fd_watch_screen_time_add_drawing(&fd_watch_screen_time.text, &fd_drawing_text_class);
    
    fd_watch_screen_time_update();
}
