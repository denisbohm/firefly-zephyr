#ifndef fd_boot_host_h
#define fd_boot_host_h

#include "fd_boot.h"

typedef struct {
    uint32_t target;
    uint32_t source;
    uint32_t system;
    uint32_t subsystem;

    uint32_t identifier;
    fd_boot_version_t version;

    void (*transmit)(const uint8_t *data, uint32_t length);

    fd_boot_update_interface_t update_interface;
} fd_boot_host_configuration_t;

void fd_boot_host_set_configuration_defaults(fd_boot_host_configuration_t *configuration);

bool fd_boot_host_start(fd_boot_host_configuration_t *configuration, fd_boot_error_t *error);

// Pass serial data received via interrupt handler to this function.
void fd_boot_host_received(const uint8_t *data, uint32_t length);

typedef enum {
    // These operations are used by the host to "rpc" into the boot loader.
    fd_boot_host_operation_get_identity = 0,
    fd_boot_host_operation_get_executable_metadata = 1,
    fd_boot_host_operation_get_update_metadata = 2,
    fd_boot_host_operation_update = 3,
    fd_boot_host_operation_execute = 4,
    // These operations are used by the boot loader to "rpc" into the host.
    fd_boot_host_operation_get_update_storage = 5,
    fd_boot_host_operation_update_read = 6,
} fd_boot_host_operation_t;

#endif