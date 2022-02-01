#ifndef fd_rtos_h
#define fd_rtos_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fd_rtos_callback_t)(void *context);

typedef struct {
    // interrupt enable/disable for critical sections inside the rtos
    uint32_t (*interrupt_disable)(void);
    void (*interrupt_enable)(uint32_t state);

    // put the MCU to sleep and wait for interrupts
    void (*sleep)(void);

    // schedule a callback to happen at the end of the given interval
    void (*schedule_callback)(fd_rtos_callback_t callback, void *context, float interval);
} fd_rtos_platform_t;

typedef void (*fd_rtos_entry_point_t)(void);

typedef struct {
    uint32_t value;
    void *task;
} fd_rtos_condition_t;

void fd_rtos_initialize(fd_rtos_platform_t *platform);
void fd_rtos_add_task(fd_rtos_entry_point_t entry_point, void *stack, size_t stack_size, uint32_t priority);
void fd_rtos_run(void);

// the platform must call this from the pendsv interrupt to schedule rtos tasks
void fd_rtos_schedule(void);

// ----- The following functions are provided for rtos tasks -----

void fd_rtos_yield(void);
void fd_rtos_delay(float duration);

uint32_t fd_rtos_interrupt_disable(void);
void fd_rtos_interrupt_enable(uint32_t state);

void fd_rtos_condition_initialize(fd_rtos_condition_t *condition);
void fd_rtos_condition_lock(fd_rtos_condition_t *condition);
void fd_rtos_condition_wait(fd_rtos_condition_t *condition, uint32_t state);
void fd_rtos_condition_signal(fd_rtos_condition_t *condition);
void fd_rtos_condition_unlock(fd_rtos_condition_t *condition);

#endif
