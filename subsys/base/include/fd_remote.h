#ifndef fd_remote_h
#define fd_remote_h

#include "fd_binary.h"

#include <stdlib.h>

typedef bool (*fd_remote_call_t)(
    void *context,
    uint8_t target,
    uint8_t source,
    uint8_t system,
    uint8_t subsystem,
    fd_binary_t *request,
    fd_binary_t *response
);

typedef bool (*fd_remote_send_t)(
    void *context,
    uint8_t target,
    uint8_t source,
    uint8_t system,
    uint8_t subsystem,
    fd_binary_t *request
);

bool fd_remote_copy_string(fd_binary_string_t source, char *destination, size_t size);

#endif