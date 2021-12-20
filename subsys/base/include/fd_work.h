#ifndef fd_work_h
#define fd_work_h

#include <stdbool.h>
#include <stdint.h>

typedef void (*fd_work_task_t)(void);

typedef struct {
    fd_work_task_t task;
    float delay;
} fd_work_item_t;

typedef struct {
    fd_work_item_t *items;
    uint32_t limit;
    uint32_t count;
} fd_work_queue_t;

void fd_work_queue_initialize(fd_work_queue_t *queue, fd_work_item_t *items, uint32_t limit);

void fd_work_queue_clear(fd_work_queue_t *queue);

void fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task, float delay);

bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed);

#endif