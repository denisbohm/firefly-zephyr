#ifndef fd_boot_split_h
#define fd_boot_split_h

typedef enum {
    // These operations are used by the host to "rpc" into the boot loader.
    fd_boot_split_operation_get_identity = 0,
    fd_boot_split_operation_get_executable_metadata = 1,
    fd_boot_split_operation_get_update_metadata = 2,
    fd_boot_split_operation_update = 3,
    fd_boot_split_operation_execute = 4,
    // These operations are used by the boot loader to "rpc" into the host.
    fd_boot_split_operation_get_update_storage = 5,
    fd_boot_split_operation_update_read = 6,
    fd_boot_split_operation_status_progress = 7,
} fd_boot_split_operation_t;

#endif