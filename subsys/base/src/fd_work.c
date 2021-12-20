#include "fd_work.h"

#include "fd_assert.h"

#include <string.h>

void fd_work_queue_initialize(fd_work_queue_t *queue, fd_work_item_t *items, uint32_t limit) {
    memset(queue, 0, sizeof(*queue));
    queue->items = items;
    queue->limit = limit;
}

void fd_work_queue_clear(fd_work_queue_t *queue) {
    memset(queue->items, 0, queue->limit * sizeof(fd_work_item_t));
    queue->count = 0;
}

void fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task, float delay) {
    fd_assert(queue->count < queue->limit);
    if (queue->count >= queue->limit) {
        return;
    }
    fd_work_item_t *item = &queue->items[queue->count++];
    item->task = task;
    item->delay = delay;
}

bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed) {
    bool any_work_left = false;
    for (uint32_t i = 0; i < queue->count; ++i) {
        fd_work_item_t *item = &queue->items[i];
        if (item->delay <= 0.0f) {
            continue;
        }
        item->delay -= elapsed;
        if (item->delay <= 0.0f) {
            item->task();
            continue;
        }
        any_work_left = true;
    }
    return any_work_left;
}