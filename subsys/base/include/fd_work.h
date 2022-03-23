#ifndef fd_work_h
#define fd_work_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void (*function)(void *context);
    void *context;
} fd_work_task_t;

typedef struct {
    const char *name;
    size_t stack_size;
    int priority;
    uint32_t task_limit;
} fd_work_queue_configuration_t;

typedef struct {
    uint32_t identifier;
} fd_work_queue_t;

typedef enum {
    fd_work_queue_submit_result_queued_already,
    fd_work_queue_submit_result_queued,
    fd_work_queue_submit_result_queued_while_running,
} fd_work_queue_submit_result_t;

typedef enum {
    fd_work_queue_wait_result_waited,
    fd_work_queue_wait_result_no_wait,
} fd_work_queue_wait_result_t;

void fd_work_initialize(void);

void fd_work_queue_initialize(fd_work_queue_t *queue, const fd_work_queue_configuration_t *configuration);

void fd_work_queue_clear(fd_work_queue_t *queue);

fd_work_queue_submit_result_t fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task);

fd_work_queue_submit_result_t fd_work_queue_submit_with_delay(fd_work_queue_t *queue, fd_work_task_t task, float delay);

fd_work_queue_wait_result_t fd_work_queue_cancel_task(fd_work_queue_t *queue, fd_work_task_t task);

fd_work_queue_wait_result_t fd_work_queue_wait_for_task(fd_work_queue_t *queue, fd_work_task_t task);

fd_work_queue_wait_result_t fd_work_queue_wait(fd_work_queue_t *queue);

bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed);

#endif