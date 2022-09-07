#ifndef fd_boot_split_peripheral_h
#define fd_boot_split_peripheral_h

#include "fd_boot.h"
#include "fd_system.h"

typedef struct {
    void *context;
    void (*initialize)(void *context);
    void (*update)(void *context);
    bool (*has_timed_out)(void *context);
    void (*finalize)(void *context);
} fd_boot_split_peripheral_timer_t;

typedef struct {
    uint32_t target;
    uint32_t source;
    uint32_t system;
    uint32_t subsystem;

    fd_system_identity_t identity;

    void (*transmit)(const uint8_t *data, uint32_t length);
    void (*aftercare)(void);
    fd_boot_split_peripheral_timer_t timer;
    bool return_on_error;

    fd_boot_update_interface_t update_interface;
} fd_boot_split_peripheral_configuration_t;

bool fd_boot_split_peripheral_set_configuration_defaults(fd_boot_split_peripheral_configuration_t *configuration, fd_boot_error_t *error);

void fd_boot_split_peripheral_initialize(fd_boot_split_peripheral_configuration_t *configuration);

bool fd_boot_split_peripheral_run(fd_boot_error_t *error);

// Pass serial data received via interrupt handler to this function.
void fd_boot_split_peripheral_received(const uint8_t *data, uint32_t length);

#endif