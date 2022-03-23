#ifndef fd_counter_h
#define fd_counter_h

#include <stdint.h>

void fd_counter_initialize(void);

double fd_counter_get_uptime(void);

typedef struct {
    void (*function)(void *context);
    void *context;
    float interval;
    uint32_t identifier;
} fd_counter_task_t;

void fd_counter_task_start(fd_counter_task_t *task);

#endif