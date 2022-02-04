#include "fd_boot_host.h"

#include "fd_binary.h"
#include "fd_boot.h"
#include "fd_dispatch.h"
#include "fd_envelope.h"
#include "fd_fifo.h"
#include "fd_unused.h"

#include <string.h>

#define fd_boot_host_message_limit 300

typedef struct {
    fd_boot_info_update_storage_t *storage;
} fd_boot_host_get_update_storage_operation_t;

typedef struct {
    uint32_t location;
    uint8_t *data;
    uint32_t length;
    fd_boot_error_t *error;
    bool result;
} fd_boot_host_update_read_operation_t;

typedef struct {
    fd_boot_host_configuration_t configuration;

    fd_fifo_t fifo;
    uint8_t fifo_buffer[fd_boot_host_message_limit];

    union {
        void *any;
        fd_boot_host_get_update_storage_operation_t *get_update_storage;
        fd_boot_host_update_read_operation_t *update_read;
    } operation;
} fd_boot_host_t;

fd_boot_host_t fd_boot_host;

void fd_boot_host_received(const uint8_t *data, uint32_t length) {
    for (uint32_t i = 0; i < length; ++i) {
        fd_fifo_put(&fd_boot_host.fifo, data[i]);
    }
}

bool fd_boot_host_rx(fd_binary_t *message) {
    fd_fifo_t *fifo = &fd_boot_host.fifo;
    uint8_t byte = 0;
    while (fd_fifo_get(fifo, &byte)) {
        if (byte == 0) {
            if (message->put_index != 0) {
                return true;
            }
        } else {
            fd_binary_put_uint8(message, byte);
        }
    }
    return false;
}

bool fd_boot_host_io(void) {
    uint8_t buffer[fd_boot_host_message_limit];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    while (!fd_boot_host_rx(&message)) {
    }

    fd_envelope_t envelope;
    bool result = fd_envelope_decode(&message, &envelope);
    if (!result) {
        return false;
    }
    message.size = message.put_index;
    return fd_dispatch_process(&message, &envelope);
}

bool fd_boot_host_tx(fd_binary_t *message) {
    fd_envelope_t envelope = {
        .target = fd_boot_host.configuration.target,
        .source = fd_boot_host.configuration.source,
        .system = fd_boot_host.configuration.system,
        .subsystem = fd_boot_host.configuration.subsystem,
        .type = fd_envelope_type_request,
    };
    if (!fd_envelope_encode(message, &envelope)) {
        return false;
    }
    fd_binary_put_uint8(message, 0);
    if (message->errors) {
        return false;
    }
    wt_uart_tx(message->buffer, message->put_index);
    return true;
}

bool fd_boot_host_get_update_storage_request(void) {
    uint8_t buffer[32];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_binary_put_uint16(&message, fd_boot_host_operation_get_update_storage);
    return fd_boot_host_tx(&message);
}

bool fd_boot_host_get_update_storage(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error fd_unused) {
    fd_boot_host_get_update_storage_operation_t get_update_storage_operation = {
        .storage = storage,
    };
    fd_boot_host.operation.get_update_storage = &get_update_storage_operation;

    if (!fd_boot_host_get_update_storage_request()) {
        fd_boot_set_error(error, 1);
        return false;
    }

    // beware: this is a nested dispatch loop... -denis
    while (fd_boot_host.operation.any != 0) {
        // !!! add timeout...
        fd_boot_host_io();
    }

    return true;
}

bool fd_boot_host_get_update_storage_response(fd_binary_t *message) {
    if (fd_boot_host.operation.get_update_storage == 0) {
        // response received with no pending operation
        return true;
    }

    fd_boot_host.operation.get_update_storage->storage->range.location = fd_binary_get_uint32(message);
    fd_boot_host.operation.get_update_storage->storage->range.length = fd_binary_get_uint32(message);
    fd_boot_host.operation.get_update_storage = 0;
    return true;
}

bool fd_boot_host_update_read_request(uint32_t location, uint32_t length) {
    uint8_t buffer[32];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_binary_put_uint16(&message, fd_boot_host_operation_update_read);
    fd_binary_put_uint32(&message, location);
    fd_binary_put_uint16(&message, (uint16_t)length);
    return fd_boot_host_tx(&message);
}

bool fd_boot_host_update_read(
    void *context fd_unused,
    uint32_t location,
    uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
) {
    fd_boot_host_update_read_operation_t update_read_operation = {
        .location = location,
        .data = data,
        .length = length,
        .error = error,
        .result = false,
    };
    fd_boot_host.operation.update_read = &update_read_operation;

    if (!fd_boot_host_update_read_request(location, length)) {
        fd_boot_set_error(error, 1);
        return false;
    }

    // beware: this is a nested dispatch loop... -denis
    while (fd_boot_host.operation.any != 0) {
        // !!! add timeout...
        fd_boot_host_io();
    }

    return update_read_operation.result;
}

bool fd_boot_host_update_read_response(fd_binary_t *message) {
    if (fd_boot_host.operation.update_read == 0) {
        // response received with no pending operation
        return true;
    }

    uint32_t location = fd_binary_get_uint32(message);
    if (location != fd_boot_host.operation.update_read->location) {
        fd_boot_set_error(fd_boot_host.operation.update_read->error, 1);
        fd_boot_host.operation.update_read->result = false;
        return true;
    }
    uint32_t length = fd_binary_get_uint16(message);
    if (length != fd_boot_host.operation.update_read->length) {
        fd_boot_set_error(fd_boot_host.operation.update_read->error, 1);
        fd_boot_host.operation.update_read->result = false;
        return true;
    }
    bool result = fd_binary_get_uint8(message) != 0;
    if (!result) {
        uint32_t code = fd_binary_get_uint32(message);
        fd_boot_set_error(fd_boot_host.operation.update_read->error, code);
        fd_boot_host.operation.update_read->result = false;
        return true;
    }
    if ((message->size - message->get_index) != length) {
        fd_boot_set_error(fd_boot_host.operation.update_read->error, 1);
        fd_boot_host.operation.update_read->result = false;
        return true;
    }
    memcpy(fd_boot_host.operation.update_read->data, &message->buffer[message->get_index], length);
    fd_boot_host.operation.update_read->result = true;
    fd_boot_host.operation.update_read = 0;
    return true;
}

bool fd_boot_host_dispatch_respond(fd_binary_t *message, fd_envelope_t *envelope) {
    if (!fd_envelope_encode(message, envelope)) {
        return false;
    }
    fd_binary_put_uint8(message, 0);
    if (message->errors) {
        return false;
    }
    wt_uart_tx(message->buffer, message->put_index);
    return true;
}

bool fd_boot_host_get_identity(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_host_operation_get_identity);
    fd_binary_put_uint32(&response, fd_boot_host.configuration.identifier);
    fd_binary_put_uint32(&response, fd_boot_host.configuration.version.major);
    fd_binary_put_uint32(&response, fd_boot_host.configuration.version.minor);
    fd_binary_put_uint32(&response, fd_boot_host.configuration.version.patch);

    fd_envelope_t response_envelope = {
        .target = envelope->source,
        .source = envelope->target,
        .system = envelope->system,
        .subsystem = envelope->subsystem,
        .type = fd_envelope_type_response,
    };
    return respond(&response, &response_envelope);
}

bool fd_boot_host_get_executable_metadata(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_executable_metadata_result_t result = {};
    bool success = fd_boot_get_executable_metadata(&fd_boot_host.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_host_operation_get_executable_metadata);
    fd_binary_put_uint8(&response, success ? 1 : 0);
    if (!success) {
        fd_binary_put_uint32(&response, error.code);
    } else {
        fd_binary_put_uint8(&response, result.is_valid ? 1 : 0);
        if (!result.is_valid) {
            fd_binary_put_uint8(&response, result.issue);
        } else {
            const fd_boot_executable_metadata_t *metadata = &result.metadata;
            const fd_boot_version_t *version = &metadata->version;
            fd_binary_put_uint32(&response, version->major);
            fd_binary_put_uint32(&response, version->minor);
            fd_binary_put_uint32(&response, version->patch);
            fd_binary_put_uint32(&response, metadata->length);
            fd_binary_put_bytes(&response, metadata->hash.data, sizeof(metadata->hash.data));
        }
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

bool fd_boot_host_get_update_metadata(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_update_metadata_result_t result = {};
    bool success = fd_boot_get_update_metadata(&fd_boot_host.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_host_operation_get_update_metadata);
    fd_binary_put_uint8(&response, success ? 1 : 0);
    if (!success) {
        fd_binary_put_uint32(&response, error.code);
    } else {
        fd_binary_put_uint8(&response, result.is_valid ? 1 : 0);
        if (!result.is_valid) {
            fd_binary_put_uint8(&response, result.issue);
        } else {
            const fd_boot_update_metadata_t *update_metadata = &result.metadata;
            const fd_boot_executable_metadata_t *executable_metadata = &update_metadata->executable_metadata;
            const fd_boot_version_t *version = &executable_metadata->version;
            fd_binary_put_uint32(&response, version->major);
            fd_binary_put_uint32(&response, version->minor);
            fd_binary_put_uint32(&response, version->patch);
            fd_binary_put_uint32(&response, executable_metadata->length);
            fd_binary_put_bytes(&response, executable_metadata->hash.data, sizeof(executable_metadata->hash.data));

            fd_binary_put_bytes(&response, update_metadata->hash.data, sizeof(update_metadata->hash.data));
            fd_binary_put_uint32(&response, update_metadata->flags);
            fd_binary_put_bytes(&response, update_metadata->initialization_vector.data, sizeof(update_metadata->initialization_vector.data));
        }
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

bool fd_boot_host_update(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_update_result_t result = {};
    bool success = fd_boot_update(&fd_boot_host.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_host_operation_update);
    fd_binary_put_uint8(&response, success ? 1 : 0);
    if (!success) {
        fd_binary_put_uint32(&response, error.code);
    } else {
        fd_binary_put_uint8(&response, result.is_valid ? 1 : 0);
        if (!result.is_valid) {
            fd_binary_put_uint8(&response, result.issue);
        }
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

bool fd_boot_host_execute(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_executable_metadata_result_t result = {};
    bool success = fd_boot_get_executable_metadata(&fd_boot_host.configuration.update_interface, &result, &error);
    if (success && result.is_valid) {
        // this call won't return of the execute is successful
        success = fd_boot_execute(&fd_boot_host.configuration.update_interface, &error);
    }

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_host_operation_execute);
    fd_binary_put_uint8(&response, success ? 1 : 0);
    if (!success) {
        fd_binary_put_uint32(&response, error.code);
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

bool fd_boot_host_dispatch_request(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint16_t operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_host_operation_get_identity:
            return fd_boot_host_get_identity(message, envelope, respond);
        case fd_boot_host_operation_get_executable_metadata:
            return fd_boot_host_get_executable_metadata(message, envelope, respond);
        case fd_boot_host_operation_get_update_metadata:
            return fd_boot_host_get_update_metadata(message, envelope, respond);
        case fd_boot_host_operation_update:
            return fd_boot_host_update(message, envelope, respond);
        case fd_boot_host_operation_execute:
            return fd_boot_host_execute(message, envelope, respond);
        default:
            return false;
    }
}

bool fd_boot_host_dispatch_response(fd_binary_t *message, fd_envelope_t *envelope fd_unused, fd_dispatch_respond_t respond fd_unused) {
    uint16_t operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_host_operation_get_update_storage:
            return fd_boot_host_get_update_storage_response(message);
        case fd_boot_host_operation_update_read:
            return fd_boot_host_update_read_response(message);
        default:
            return false;
    }
}

bool fd_boot_host_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    if (envelope->type == fd_envelope_type_request) {
        return fd_boot_host_dispatch_request(message, envelope, respond);
    }
    if (envelope->type == fd_envelope_type_response) {
        return fd_boot_host_dispatch_response(message, envelope, respond);
    }
    return false;
}

void fd_boot_host_set_configuration_defaults(fd_boot_host_configuration_t *configuration) {
    *configuration = (fd_boot_host_configuration_t) {
        .target = 1,
        .source = 0,
        .system = 0,
        .subsystem = 0,

        .identifier = 0x626f6f74, /* boot */
        .version = {
            .major = 1,
            .minor = 0,
            .patch = 0,
        },

        .update_interface = {
            .info = {
                .get_update_storage = fd_boot_host_get_update_storage,
            },
            .update_reader = {
                .read = fd_boot_host_update_read,
            },
        },
    };
}

bool fd_boot_host_start(fd_boot_host_configuration_t *configuration, fd_boot_error_t *error) {
    memset(&fd_boot_host, 0, sizeof(fd_boot_host));
    fd_boot_host.configuration = *configuration;
    fd_fifo_initialize(&fd_boot_host.fifo, fd_boot_host.fifo_buffer, sizeof(fd_boot_host.fifo_buffer));

    fd_dispatch_initialize(fd_boot_host_dispatch_respond);
    fd_dispatch_add_process(fd_boot_host.configuration.system, fd_boot_host.configuration.subsystem, fd_boot_host_dispatch_process);

    while (true) {
        fd_boot_host_io();
    }

    fd_boot_set_error(error, 1);
    return false;
}
