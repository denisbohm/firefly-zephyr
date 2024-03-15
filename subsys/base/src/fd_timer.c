#include "fd_timer.h"

#include "fd_assert.h"

#include <string.h>

#ifndef fd_timer_limit
#define fd_timer_limit 32
#endif

typedef struct {
    fd_timer_t *timers[fd_timer_limit];
    uint32_t timer_count;
    float timestamp;
} fd_timer_singleton_t;

fd_timer_singleton_t fd_timer;

void fd_timer_initialize(void) {
    memset(&fd_timer, 0, sizeof(fd_timer));
}

float fd_timer_get_timestamp(void) {
    return fd_timer.timestamp;
}

void fd_timer_add_with_identifier(fd_timer_t *timer, fd_timer_callback_t callback, const char *identifier __attribute__((unused))) {
    timer->callback = callback;
    timer->active = false;
    timer->triggered = false;
    timer->countdown = 0.0f;

    if (fd_timer.timer_count >= fd_timer_limit) {
        fd_assert_fail("timer limit");
        return;
    }

    fd_timer.timers[fd_timer.timer_count++] = timer;
}

void fd_timer_start(fd_timer_t *timer, float duration) {
    timer->countdown = duration;
    timer->active = true;
    timer->triggered = false;
}

void fd_timer_stop(fd_timer_t *timer) {
    timer->countdown = 0.0f;
    timer->active = false;
    timer->triggered = false;
}

void fd_timer_schedule(float elapsed) {
    for (uint32_t i = 0; i < fd_timer.timer_count; ++i) {
        fd_timer_t *timer = fd_timer.timers[i];
        if (!timer->active) {
            continue;
        }
        if (timer->countdown <= elapsed) {
            timer->active = false;
            timer->triggered = true;
            timer->countdown = 0.0f;
            continue;
        }
        timer->countdown -= elapsed;
    }
}

void fd_timer_callback(void) {
    for (uint32_t i = 0; i < fd_timer.timer_count; ++i) {
        fd_timer_t *timer = fd_timer.timers[i];
        if (timer->triggered) {
            timer->triggered = false;
            (*timer->callback)(timer->context);
        }
    }
}

void fd_timer_update(float elapsed) {
    fd_timer.timestamp += elapsed;
    fd_timer_schedule(elapsed);
    fd_timer_callback();
}
