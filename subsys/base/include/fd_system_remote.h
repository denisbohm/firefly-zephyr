#ifndef fd_system_remote_h
#define fd_system_remote_h

#include "fd_remote.h"
#include "fd_version.h"

#include <stdbool.h>
#include <stdint.h>

void fd_system_remote_request_get_version(fd_binary_t *request);

bool fd_system_remote_response_get_version(
    fd_binary_t *response,
    fd_version_t *version
);

bool fd_system_remote_send_get_version(
    fd_remote_send_t remote_send, void *context, uint8_t target, uint8_t source
);

bool fd_system_remote_call_get_version(
    fd_remote_call_t remote_call, void *context, uint8_t target, uint8_t source,
    fd_version_t *version
);

typedef struct {
    char file[32];
    uint32_t line;
    char message[32];
} fd_system_remote_failure_t;

void fd_system_remote_request_get_assert(fd_binary_t *request, uint32_t limit);

bool fd_system_remote_response_get_assert(
    fd_binary_t *response,
    uint32_t limit,
    uint32_t *total, fd_system_remote_failure_t *failures, uint32_t *count
);

bool fd_system_remote_send_get_assert(
    fd_remote_send_t remote_send, void *context, uint8_t target, uint8_t source,
    uint32_t limit
);

bool fd_system_remote_call_get_assert(
    fd_remote_call_t remote_call, void *context, uint8_t target, uint8_t source,
    uint32_t limit,
    uint32_t *total, fd_system_remote_failure_t *failures, uint32_t *count
);

#endif