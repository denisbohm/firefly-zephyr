#include "fd_work.h"

#include "fd_assert.h"

#include <string.h>

#ifndef fd_work_queue_limit
#define fd_work_queue_limit 4
#endif

#ifndef fd_work_task_limit
#define fd_work_task_limit 8
#endif

typedef struct {
    fd_work_task_t task;
    bool is_queued;
    float delay;
} fd_work_item_t;

typedef struct {
    fd_work_queue_configuration_t configuration;
    fd_work_item_t items[fd_work_task_limit];
    uint32_t item_count;
} fd_work_queue_impl_t;

typedef struct {
    fd_work_queue_impl_t queues[fd_work_queue_limit];
    uint32_t queue_count;
} fd_work_t;

fd_work_t fd_work;

void fd_work_initialize(void) {
    memset(&fd_work, 0, sizeof(fd_work));
}

void fd_work_queue_initialize(fd_work_queue_t *queue, const fd_work_queue_configuration_t *configuration) {
    queue->identifier = fd_work.queue_count;
    fd_assert(fd_work.queue_count < fd_work_queue_limit);
    if (fd_work.queue_count >= fd_work_queue_limit) {
        return;
    }
    fd_assert(configuration->task_limit <= fd_work_task_limit);
    fd_work_queue_impl_t *impl = &fd_work.queues[fd_work.queue_count++];
    impl->configuration = *configuration;
}

void fd_work_queue_clear(fd_work_queue_t *queue) {
    if (queue->identifier >= fd_work_queue_limit) {
        return;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    memset(impl->items, 0, fd_work_task_limit * sizeof(fd_work_item_t));
    impl->item_count = 0;
}

fd_work_item_t *fd_work_item_for_task(fd_work_queue_impl_t *impl, fd_work_task_t task) {
    for (uint32_t i = 0; i < impl->item_count; ++i) {
        fd_work_item_t *item = &impl->items[i];
        if ((item->task.function == task.function) && (item->task.context == task.context)) {
            return item;
        }
    }
    return 0;
}

fd_work_queue_submit_result_t fd_work_queue_submit_with_delay(fd_work_queue_t *queue, fd_work_task_t task, float delay) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_submit_result_queued;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    fd_work_item_t *item = fd_work_item_for_task(impl, task);
    if (item == 0) {
        fd_assert(impl->item_count < fd_work_task_limit);
        if (impl->item_count >= fd_work_task_limit) {
            return fd_work_queue_submit_result_queued;
        }
        item = &impl->items[impl->item_count++];
        item->task = task;
    }
    if (item->is_queued) {
        return fd_work_queue_submit_result_queued_already;
    }
    item->is_queued = true;
    item->delay = delay;
    return fd_work_queue_submit_result_queued;
}

fd_work_queue_submit_result_t fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task) {
    return fd_work_queue_submit_with_delay(queue, task, 0.0f);
}

fd_work_queue_wait_result_t fd_work_queue_cancel_task(fd_work_queue_t *queue, fd_work_task_t task) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_wait_result_waited;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    fd_work_item_t *item = fd_work_item_for_task(impl, task);
    if (item != 0) {
        item->is_queued = false;
    }
    return fd_work_queue_wait_result_waited;
}

fd_work_queue_wait_result_t fd_work_queue_wait_for_task(fd_work_queue_t *queue, fd_work_task_t task) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_wait_result_waited;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    fd_work_item_t *item = fd_work_item_for_task(impl, task);
    if (item != 0) {
        if (item->is_queued) {
            item->is_queued = false;
            fd_work_task_t *task = &item->task;
            task->function(task->context);
        }
    }
    return fd_work_queue_wait_result_waited;
}

fd_work_queue_wait_result_t fd_work_queue_wait(fd_work_queue_t *queue) {
    return fd_work_queue_wait_result_waited;
}

bool fd_work_queue_impl_process(fd_work_queue_impl_t *impl, float elapsed) {
    bool any_work_left = false;
    for (uint32_t i = 0; i < impl->item_count; ++i) {
        fd_work_item_t *item = &impl->items[i];
        if (!item->is_queued) {
            continue;
        }
        item->delay -= elapsed;
        if (item->delay <= 0.0f) {
            item->is_queued = false;
            fd_work_task_t *task = &item->task;
            task->function(task->context);
            continue;
        }
        any_work_left = true;
    }
    return any_work_left;
}

bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_submit_result_queued;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    return fd_work_queue_impl_process(impl, elapsed);
}

bool fd_work_queue_process_all(float elapsed) {
    bool any_work_left = false;
    for (uint32_t i = 0; i < fd_work.queue_count; ++i) {
        fd_work_queue_impl_t *impl = &fd_work.queues[i];
        if (fd_work_queue_impl_process(impl, elapsed)) {
            any_work_left = true;
        }
    }
    return any_work_left;
}
