#ifndef fd_boot_split_controller_h
#define fd_boot_split_controller_h

#include "fd_boot_error.h"
#include "fd_boot_range.h"
#include "fd_boot_split.h"
#include "fd_boot_version.h"
#include "fd_fifo.h"
#include "fd_system.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *context;
    void (*initialize)(void *context);
    void (*update)(void *context);
    bool (*has_timed_out)(void *context);
    void (*finalize)(void *context);
} fd_boot_split_controller_timer_t;

typedef struct {
    uint32_t target;
    uint32_t source;
    uint32_t system;
    uint32_t subsystem;

    void (*get_update_storage)(fd_boot_range_t *range);
    void (*update_read)(uint32_t location, uint8_t *data, uint32_t length);
    void (*progress)(float amount);

    bool (*transmit)(const uint8_t *data, uint32_t length);
    fd_boot_split_controller_timer_t timer;
    uint32_t duration;

    fd_fifo_t fifo;
} fd_boot_split_controller_t;

bool fd_boot_split_controller_set_defaults(
    fd_boot_split_controller_t *controller,
    fd_boot_error_t *error
);

bool fd_boot_split_controller_initialize(
    fd_boot_split_controller_t *controller,
    fd_boot_error_t *error
);

bool fd_boot_split_controller_received(
    fd_boot_split_controller_t *controller,
    const uint8_t *data,
    uint32_t length
);

bool fd_boot_split_controller_get_identity(
    fd_boot_split_controller_t *controller,
    fd_version_t *version,
    char *identifier,
    size_t identifier_size,
    fd_boot_error_t *error
);

typedef struct {
    bool is_valid;
    int issue;
} fd_boot_split_controller_update_result_t;

bool fd_boot_split_controller_update(
    fd_boot_split_controller_t *controller,
    fd_boot_split_controller_update_result_t *result,
    fd_boot_error_t *error
);

typedef struct {
    bool is_valid;
    int issue;
} fd_boot_split_controller_execute_result_t;

bool fd_boot_split_controller_execute(
    fd_boot_split_controller_t *controller,
    fd_boot_split_controller_execute_result_t *result,
    fd_boot_error_t *error
);

#endif