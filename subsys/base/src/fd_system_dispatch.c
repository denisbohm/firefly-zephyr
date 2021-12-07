#include "fd_system_dispatch.h"

#include "fd_dispatch.h"

typedef enum {
    fd_system_dispatch_operation_get_version,
} fd_system_dispatch_operation_t;

bool fd_system_dispatch_get_version(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_system_version_t version = fd_system_get_version();
    fd_binary_put_uint8(&response, fd_system_dispatch_operation_get_version);
    fd_binary_put_uint32(&response, version.major);
    fd_binary_put_uint32(&response, version.minor);
    fd_binary_put_uint32(&response, version.patch);
    fd_envelope_t response_envelope = {
        .target = envelope->source,
        .source = envelope->target,
        .system = envelope->system,
        .subsystem = envelope->subsystem,
        .type = fd_envelope_type_response,
    };
    return respond(&response, &response_envelope);
}

bool fd_system_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t operation = fd_binary_get_uint8(message);
    switch (operation) {
        case fd_system_dispatch_operation_get_version:
            return fd_system_dispatch_get_version(message, envelope, respond);
        break;
        default:
        return false;
    }
}

void fd_system_dispatch_initialize(void) {
    fd_dispatch_add_process(fd_envelope_system_firefly, fd_envelope_subsystem_system, fd_system_dispatch_process);
}