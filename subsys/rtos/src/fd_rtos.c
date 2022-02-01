#include "fd_rtos.h"

#include <string.h>

typedef struct {
    uint32_t stack_pointer;
    uint32_t stack_base;
    uint32_t stack_size;
    uint32_t priority;
    bool runnable;
} fd_rtos_task_t;

typedef struct {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r14;
} fd_rtos_software_stack_frame_t;

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} fd_rtos_hardware_stack_frame_t;

typedef struct {
    fd_rtos_software_stack_frame_t software;
    fd_rtos_hardware_stack_frame_t hardware;
} fd_rtos_stack_frame_t;

#ifndef fd_rtos_task_limit
#define fd_rtos_task_limit 4
#endif

typedef struct {
    fd_rtos_platform_t *platform;
    uint32_t interrupt_level;
    fd_rtos_task_t tasks[fd_rtos_task_limit];
    uint32_t task_count;
    uint32_t task_index;
    bool task_start;
    uint32_t assertion_failure_count;

    uint8_t sleep_stack[1024] __attribute__((aligned(8)));
} fd_rtos_t;

fd_rtos_t fd_rtos;

void fd_rtos_assert(bool value) {
    if (!value) {
        ++fd_rtos.assertion_failure_count;
    }
}

bool fd_rtos_is_any_task_runnable(void) {
    // skip sleep task, which is always runnable
    for (uint32_t i = 1; i < fd_rtos.task_count; ++i) {
        fd_rtos_task_t *task = &fd_rtos.tasks[i];
        if (task->runnable) {
            return true;
        }
    }
    return false;
}

void fd_rtos_sleep_task(void) {
    while (true) {
        uint32_t state = fd_rtos.platform->interrupt_disable();
        if (!fd_rtos_is_any_task_runnable()) {
            platform->sleep();
        }
        fd_rtos.platform->interrupt_enable(state);
        fd_rtos_yield();
    }
}

void fd_rtos_initialize(fd_rtos_platform_t *platform) {
    memset(&fd_rtos, 0, sizeof(fd_rtos));
    fd_rtos.platform = platform;
    fd_rtos.task_start = true;

    NVIC_SetPriority(PendSV_IRQn, 2);

    fd_rtos_add_task(fd_rtos_sleep_task, fd_rtos.sleep_stack, sizeof(fd_rtos.sleep_stack), 0);
}

uint32_t fd_rtos_interrupt_disable(void) {
    return fd_rtos.platform->interrupt_disable();
}

void fd_rtos_interrupt_enable(uint32_t state) {
    fd_rtos.platform->interrupt_enable(state);
}

void fd_rtos_task_fallthrough(void) {
    fd_rtos_assert(false);
    while (true) {
    }
}

void fd_rtos_add_task(fd_rtos_entry_point_t entry_point, void *stack, size_t stack_size, uint32_t priority) {
    if (fd_rtos.task_count >= fd_rtos_task_size) {
        return;
    }
    memset(stack, 0, stack_size);
    fd_rtos_task_t *task = &fd_rtos.tasks[fd_rtos.task_count];
    task->stack_pointer = ((uint32_t)stack) + stack_size;
    task->stack_pointer -= sizeof(fd_rtos_stack_frame_t);
    task->stack_base = (uint32_t)stack;
    task->stack_size = stack_size;
    fd_rtos_stack_frame_t *stack_frame = (fd_rtos_stack_frame_t *)task->stack_pointer;
    memset(stack_frame, 0, sizeof(stack_frame));
    stack_frame->software.r14 = 0xfffffffd;
    stack_frame->hardware.lr = (uint32_t)fd_rtos_task_fallthrough;
    stack_frame->hardware.pc = (uint32_t)entry_point;
    stack_frame->hardware.psr = 0x21000000;
    task->priority = priority;
    task->runnable = true;
    ++fd_rtos.task_count;
}

// call from pendsv interrupt
void fd_rtos_schedule(void) {
    if (fd_rtos.task_start) {
        fd_rtos.task_start = false;
    } else {
        // suspend current task
        uint32_t stack_pointer;
        __asm(
            "mrs r0, psp\n"
            "tst r14, #0x10\n"
            "it eq\n"
            "vstmdbeq	r0!, {s16-s31}\n"
            "stmdb r0!, {r4-r11, r14}\n"
            "mov %[stack_pointer], r0\n"
            : [stack_pointer] "=r" (stack_pointer)
        );
        fd_rtos_task_t *task = &fd_rtos.tasks[fd_rtos.task_index];
        task->stack_pointer = stack_pointer;
        uint32_t top = task->stack_base + task->stack_size;
        fd_rtos_assert((task->stack_base <= stack_pointer) && (stack_pointer < top));
    }

    // find new task to run
    fd_rtos.task_index = 0;
    fd_rtos_task_t *task = &fd_rtos.tasks[fd_rtos.task_index]; // sleep task is always runnable
    for (uint32_t i = 1; i < fd_rtos.task_count; ++i) {
        uint32_t index = fd_rtos.task_index + i;
        if (index >= fd_rtos.task_count) {
            index -= fd_rtos.task_count;
        }
        fd_rtos_task_t *candidate = &fd_rtos.tasks[index];
        if (candidate->runnable && (candidate->priority > task->priority)) {
            task = candidate;
            fd_rtos.task_index = index;
        }
    }

    // resume task to run
    uint32_t stack_pointer = task->stack_pointer;
    uint32_t top = task->stack_base + task->stack_size;
    fd_rtos_assert((task->stack_base <= stack_pointer) && (stack_pointer < top));
    __asm(
        "mov r0, %[stack_pointer]\n"
        "ldmia r0!, {r4-r11, r14}\n"
        "tst r14, #0x10\n"
        "it eq\n"
        "vldmiaeq r0!, {s16-s31}\n"
        "msr psp, r0\n"
        "bx r14\n"
        :
        : [stack_pointer] "r" (stack_pointer)
    );
}

void fd_rtos_run(void) {
    fd_rtos_assert(fd_rtos.task_start);
    fd_rtos_yield();
    fd_rtos_assert(false); // should never get here
    fd_rtos_task_fallthrough();
}

void fd_rtos_yield(void) {
    // invoke scheduler exception handler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void fd_rtos_delay_callback(void *context) {
    fd_rtos_task_t *task = context;
    task->runnable = true;
}

void fd_rtos_delay(float duration) {
    uint32_t state = fd_rtos.platform->interrupt_disable();
    fd_rtos_task_t *task = &fd_rtos.tasks[fd_rtos.delay_task_index];
    task->runnable = false;
    fd_rtos.platform->interrupt_enable(state);
    fd_rtos.platform->schedule_callback(fd_rtos_delay_callback, task, duration);
    fd_rtos_yield();
}

bool fd_rtos_interrupt_context(void) {
    uint32_t value;
    __asm(
	" mrs %[value], ipsr"
        : [value] "=r" (value));
    return value != 0;
}

void fd_rtos_condition_initialize(fd_rtos_condition_t *condition) {
    memset(condition, 0, sizeof(fd_rtos_condition_t));
}

void fd_rtos_condition_lock(fd_rtos_condition_t *condition) {
#if 0
    uint32_t *value_pointer = &condition->value;
    __asm(
        " MOV R1, #1\n"
        "fd_rtos_spin_lock:\n"
        " LDREX r0, [%[value_pointer]]\n"
        " CMP r0, r1\n"
        " IT NE\n"
        " STREXNE r0, r1, [%[value_pointer]]\n"
        " ITE NE\n"
        " CMPNE r0, #1\n"
        " BEQ fd_rtos_spin_lock\n"
        " DMB\n"
        : [value_pointer] "=r" (value_pointer));
#endif
}

void fd_rtos_condition_wait(fd_rtos_condition_t *condition, uint32_t state) {
    fd_rtos_assert(!fd_rtos_interrupt_context());
    fd_rtos_assert(condition->task == 0);
    fd_rtos_task_t *task = &fd_rtos.tasks[fd_rtos.task_index];
    condition->task = task;
    task->runnable = false;
    fd_rtos.platform->interrupt_enable(state);
    fd_rtos_yield();
}

void fd_rtos_condition_signal(fd_rtos_condition_t *condition) {
    fd_rtos_assert(fd_rtos_interrupt_context());
    fd_rtos_task_t *task = (fd_rtos_task_t *)condition->task;
    if (task != 0) {
        task->runnable = true;
        condition->task = 0;
    }
}

void fd_rtos_condition_unlock(fd_rtos_condition_t *condition) {
#if 0
    uint32_t *value_pointer = &condition->value;
    __asm(
        " MOV R1, #0\n"
        " DMB\n"
        " STR r1, [%[value_pointer]]\n"
        : [value_pointer] "=r" (value_pointer));
#endif
}