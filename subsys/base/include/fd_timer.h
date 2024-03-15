#ifndef fd_timer_h
#define fd_timer_h

#include <stdbool.h>
#include <stdint.h>

typedef void (*fd_timer_callback_t)(void *context);

typedef struct {
    void *context;
    fd_timer_callback_t callback;
    float countdown;
    bool active;
    bool triggered;
} fd_timer_t;

void fd_timer_initialize(void);
void fd_timer_update(float elapsed);
float fd_timer_get_timestamp(void);

void fd_timer_add_with_identifier(fd_timer_t *timer, fd_timer_callback_t callback, const char *identifier);
#define fd_timer_add(timer, callback) fd_timer_add_with_identifier(timer, callback, #callback)
void fd_timer_start(fd_timer_t *timer, float duration);
void fd_timer_start_next(fd_timer_t *timer, float interval);
void fd_timer_stop(fd_timer_t *timer);

#endif
