#include "fd_system_remote.h"

#include "fd_binary.h"
#include "fd_envelope.h"
#include "fd_system_operation.h"

#include <string.h>

void fd_system_remote_request_get_version(fd_binary_t *request) {
    fd_binary_put_uint8(request, fd_system_operation_get_identity);
}

bool fd_system_remote_response_get_version(
    fd_binary_t *response,
    fd_version_t *version
) {
    uint8_t operation = fd_binary_get_uint8(response);
    if (operation != fd_system_operation_get_identity) {
        return false;
    }
    version->major = fd_binary_get_uint32(response);
    version->minor = fd_binary_get_uint32(response);
    version->patch = fd_binary_get_uint32(response);
    return response->errors == 0;
}

bool fd_system_remote_call_get_version(
    fd_remote_call_t remote_call, void *context, uint8_t target, uint8_t source,
    fd_version_t *version
) {
    uint8_t buffer[64];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_system_remote_request_get_version(&message);
    if (!remote_call(context, target, source, fd_envelope_system_firefly, fd_envelope_subsystem_system, &message, &message)) {
        return false;
    }
    return fd_system_remote_response_get_version(&message, version);
}

void fd_system_remote_request_get_assert(fd_binary_t *request, uint32_t limit) {
    fd_binary_put_uint8(request, fd_system_operation_get_assert);
}

bool fd_system_remote_response_get_assert(
    fd_binary_t *response,
    uint32_t limit,
    uint32_t *total, fd_system_remote_failure_t *failures, uint32_t *count
) {
    *total = fd_binary_get_uint32(response);
    *count = 0;
    uint8_t operation = fd_binary_get_uint8(response);
    if (operation != fd_system_operation_get_assert) {
        return false;
    }
    fd_system_remote_failure_t *failure = failures;
    while ((response->get_index < response->put_index) && (*count < limit)) {
        fd_binary_string_t file = fd_binary_get_string(response);
        fd_remote_copy_string(file, failure->file, sizeof(failure->file));
        failure->line = fd_binary_get_uint32(response);
        fd_binary_string_t text = fd_binary_get_string(response);
        fd_remote_copy_string(text, failure->message, sizeof(failure->message));
        if (response->errors != 0) {
            return false;
        }
        ++*count;
        ++failure;
    }
    return true;
}

bool fd_system_remote_call_get_assert(
    fd_remote_call_t remote_call, void *context, uint8_t target, uint8_t source,
    uint32_t limit,
    uint32_t *total, fd_system_remote_failure_t *failures, uint32_t *count
) {
    uint8_t buffer[128];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_system_remote_request_get_assert(&message, limit);
    if (!remote_call(context, target, source, fd_envelope_system_firefly, fd_envelope_subsystem_system, &message, &message)) {
        return false;
    }
    return fd_system_remote_response_get_assert(&message, limit, total, failures, count);
}