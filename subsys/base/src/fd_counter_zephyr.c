#include "fd_counter.h"

#include <zephyr.h>

#include <string.h>

#ifndef fd_counter_task_limit
#define fd_counter_task_limit 1
#endif

typedef struct {
    struct k_timer timer;
    fd_counter_task_t *task;
} fd_counter_item_t;

typedef struct {
    fd_counter_item_t items[fd_counter_task_limit];
    uint32_t item_count;
} fd_counter_t;

fd_counter_t fd_counter;

void fd_counter_task_irq(struct k_timer *timer) {
    fd_counter_item_t *item = CONTAINER_OF(timer, fd_counter_item_t, timer);
    item->task->function(item->task->context);
}

void fd_counter_initialize(void) {
    memset(&fd_counter, 0, sizeof(fd_counter_t));
    for (uint32_t i = 0; i < fd_counter_task_limit; ++i) {
        k_timer_init(&fd_counter.items[i].timer, fd_counter_task_irq, NULL);
    }
}

double fd_counter_get_uptime(void) {
    return (double)k_uptime_get() / 1000.0;
}

void fd_counter_task_start(fd_counter_task_t *task) {
    task->identifier = fd_counter.item_count;
    if (fd_counter.item_count >= fd_counter_task_limit) {
        return;
    }
    fd_counter_item_t *item = &fd_counter.items[fd_counter.item_count++];
    item->task = task;
    k_timeout_t ms = K_MSEC((int)(task->interval * 1000.0));
    k_timer_start(&item->timer, ms, ms);
}
