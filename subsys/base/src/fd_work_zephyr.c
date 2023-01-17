#include "fd_work.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>

#include <math.h>
#include <string.h>

#ifndef fd_work_queue_limit
#define fd_work_queue_limit 8
#endif

#ifndef fd_work_task_limit
#define fd_work_task_limit 16
#endif

#ifndef fd_work_stack_limit
#define fd_work_stack_limit 32768
#endif

typedef struct {
    fd_work_task_t task;
    struct k_work_delayable work_delayable;
} fd_work_item_t;

typedef struct {
    fd_work_queue_configuration_t configuration;
    fd_work_item_t items[fd_work_task_limit];
    uint32_t item_count;
    struct k_work_q work_queue;
} fd_work_queue_impl_t;

K_THREAD_STACK_DEFINE(fd_work_stack, fd_work_stack_limit);

#define fd_work_task_history_limit 256

typedef struct {
    fd_work_queue_impl_t queues[fd_work_queue_limit];
    uint32_t queue_count;
    uint32_t stack_used;

    fd_work_task_t history[fd_work_task_history_limit];
    uint32_t history_count;
} fd_work_t;

fd_work_t fd_work;

void fd_work_initialize(void) {
    memset(&fd_work, 0, sizeof(fd_work));
}

void fd_work_run(struct k_work *work) {
    struct k_work_delayable *work_delayable = k_work_delayable_from_work(work);
    fd_work_item_t *item = CONTAINER_OF(work_delayable, fd_work_item_t, work_delayable);
    item->task.function(item->task.context);
}

void fd_work_queue_initialize(fd_work_queue_t *queue, const fd_work_queue_configuration_t *configuration) {
    queue->identifier = fd_work.queue_count;
    fd_assert(fd_work.queue_count < fd_work_queue_limit);
    if (fd_work.queue_count >= fd_work_queue_limit) {
        return;
    }
    if ((fd_work.stack_used + configuration->stack_size) > sizeof(fd_work_stack)) {
        fd_assert_fail("not enough stack");
        return;
    }
    fd_assert(configuration->task_limit <= fd_work_task_limit);
    fd_work_queue_impl_t *impl = &fd_work.queues[fd_work.queue_count++];
    impl->configuration = *configuration;

    for (uint32_t i = 0; i < fd_work_task_limit; ++i) {
        k_work_init_delayable(&impl->items[i].work_delayable, fd_work_run);
    }
    k_work_queue_init(&impl->work_queue);
    const struct k_work_queue_config sampling_work_queue_configuration = {
        .name = configuration->name,
    };
    struct z_thread_stack_element *stack = &fd_work_stack[fd_work.stack_used];
    fd_work.stack_used += configuration->stack_size;
    k_work_queue_start(
        &impl->work_queue,
        stack,
        configuration->stack_size,
        configuration->priority,
        &sampling_work_queue_configuration
    );
}

void fd_work_queue_clear(fd_work_queue_t *queue) {
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
    if (fd_work.history_count < fd_work_task_history_limit) {
        fd_work.history[fd_work.history_count++] = task;
    }

    fd_assert(task.function != 0);
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
    k_timeout_t timeout = delay == 0.0f ? K_NO_WAIT : K_MSEC(ceilf(delay * 1000.0f));
    int result = k_work_reschedule_for_queue(&impl->work_queue, &item->work_delayable, timeout);
    switch (result) {
        case 0: return fd_work_queue_submit_result_queued_already;
        case 1: return fd_work_queue_submit_result_queued;
        case 2: return fd_work_queue_submit_result_queued_while_running;
        default: return fd_work_queue_submit_result_not_queued;
    }
}

fd_work_queue_submit_result_t fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task) {
    return fd_work_queue_submit_with_delay(queue, task, 0.0f);
}

fd_work_queue_wait_result_t fd_work_queue_cancel_task(fd_work_queue_t *queue, fd_work_task_t task) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_wait_result_no_wait;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    fd_work_item_t *item = fd_work_item_for_task(impl, task);
    if (item == 0) {
        return fd_work_queue_wait_result_no_wait;
    }
    bool result = k_work_cancel_delayable(&item->work_delayable);
    return result ? fd_work_queue_wait_result_waited : fd_work_queue_wait_result_no_wait;
}

fd_work_queue_wait_result_t fd_work_queue_wait_for_task(fd_work_queue_t *queue, fd_work_task_t task) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_wait_result_no_wait;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    fd_work_item_t *item = fd_work_item_for_task(impl, task);
    if (item == 0) {
        return fd_work_queue_wait_result_no_wait;
    }
    struct k_work_sync work_sync;
    bool result = k_work_flush_delayable(&item->work_delayable, &work_sync);
    return result ? fd_work_queue_wait_result_waited : fd_work_queue_wait_result_no_wait;
}

fd_work_queue_wait_result_t fd_work_queue_wait(fd_work_queue_t *queue) {
    fd_assert(queue->identifier < fd_work_queue_limit);
    if (queue->identifier >= fd_work_queue_limit) {
        return fd_work_queue_submit_result_queued;
    }
    fd_work_queue_impl_t *impl = &fd_work.queues[queue->identifier];
    int result = k_work_queue_drain(&impl->work_queue, false);
    fd_assert(result >= 0);
    switch (result) {
        case 0: return fd_work_queue_wait_result_waited;
        case 1: return fd_work_queue_wait_result_no_wait;
    }
    return fd_work_queue_wait_result_waited;
}

void fd_work_sleep(float seconds) {
    uint32_t usec = (int)ceil(seconds * 1000000.0f);
    k_sleep(K_USEC(usec));
}

bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed) {
    return false;
}