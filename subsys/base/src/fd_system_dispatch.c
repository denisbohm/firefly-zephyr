#include "fd_system_dispatch.h"

#include "fd_assert.h"
#include "fd_dispatch.h"

#include <string.h>

typedef enum {
    fd_system_dispatch_operation_get_identity,
    fd_system_dispatch_operation_get_assert,
} fd_system_dispatch_operation_t;

typedef struct {
    const char *file;
    int line;
    char message[32];
} fd_system_dispatch_t;

fd_system_dispatch_t fd_system_dispatch;

bool fd_system_dispatch_get_identity(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t buffer[64];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    const fd_system_identity_t *identity = fd_system_get_identity();
    fd_binary_put_uint8(&response, fd_system_dispatch_operation_get_identity);
    fd_binary_put_uint32(&response, identity->version.major);
    fd_binary_put_uint32(&response, identity->version.minor);
    fd_binary_put_uint32(&response, identity->version.patch);
    fd_binary_put_string(&response, identity->identifier);
    fd_envelope_t response_envelope = {
        .target = envelope->source,
        .source = envelope->target,
        .system = envelope->system,
        .subsystem = envelope->subsystem,
        .type = fd_envelope_type_response,
    };
    return respond(&response, &response_envelope);
}

bool fd_system_dispatch_get_assert(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t buffer[64];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, fd_system_dispatch_operation_get_assert);
    fd_binary_put_uint32(&response, fd_assert_get_count());
    const char *file = fd_system_dispatch.file;
    if (file) {
        char *slash = strrchr(file, '/');
        if (slash) {
            file = slash + 1;
        }
        fd_binary_put_string(&response, file);
        fd_binary_put_uint32(&response, fd_system_dispatch.line);
        fd_binary_put_string(&response, fd_system_dispatch.message);
    }
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
        case fd_system_dispatch_operation_get_identity:
            return fd_system_dispatch_get_identity(message, envelope, respond);
        case fd_system_dispatch_operation_get_assert:
            return fd_system_dispatch_get_assert(message, envelope, respond);
        default:
        return false;
    }
}

void fd_system_assert_callback(const char *file, int line, const char *message) {
    if (fd_system_dispatch.file != 0) {
        return;
    }

    fd_system_dispatch.file = file;
    fd_system_dispatch.line = line;
    strncpy(fd_system_dispatch.message, message, sizeof(fd_system_dispatch.message));
    fd_system_dispatch.message[sizeof(fd_system_dispatch.message) - 1] = '\0';
}

void fd_system_dispatch_initialize(void) {
    memset(&fd_system_dispatch, 0, sizeof(fd_system_dispatch));

    fd_assert_set_callback(fd_system_assert_callback);

    fd_dispatch_add_process(fd_envelope_system_firefly, fd_envelope_subsystem_system, fd_system_dispatch_process);
}