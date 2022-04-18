#include "fd_boot_split_peripheral.h"

#include "fd_binary.h"
#include "fd_boot.h"
#include "fd_boot_split.h"
#include "fd_dispatch.h"
#include "fd_envelope.h"
#include "fd_fifo.h"
#include "fd_unused.h"

#include <string.h>

#ifndef fd_boot_split_peripheral_message_limit
#define fd_boot_split_peripheral_message_limit 300
#endif

#ifndef fd_boot_split_peripheral_timeout
#define fd_boot_split_peripheral_timeout 100000
#endif

typedef struct {
    fd_boot_info_update_storage_t *storage;
} fd_boot_split_peripheral_get_update_storage_operation_t;

typedef struct {
    uint32_t location;
    uint8_t *data;
    uint32_t length;
    fd_boot_error_t *error;
    bool result;
} fd_boot_split_peripheral_update_read_operation_t;

typedef struct {
    fd_boot_split_peripheral_configuration_t configuration;

    fd_fifo_t fifo;
    uint8_t fifo_buffer[fd_boot_split_peripheral_message_limit];

    uint32_t duration;

    union {
        void *any;
        fd_boot_split_peripheral_get_update_storage_operation_t *get_update_storage;
        fd_boot_split_peripheral_update_read_operation_t *update_read;
    } operation;
} fd_boot_split_peripheral_t;

fd_boot_split_peripheral_t fd_boot_split_peripheral;

void fd_boot_split_peripheral_received(const uint8_t *data, uint32_t length) {
    for (uint32_t i = 0; i < length; ++i) {
        fd_fifo_put(&fd_boot_split_peripheral.fifo, data[i]);
    }
}

bool fd_boot_split_peripheral_poll(fd_binary_t *message) {
    fd_fifo_t *fifo = &fd_boot_split_peripheral.fifo;
    uint8_t byte = 0;
    while (fd_fifo_get(fifo, &byte)) {
        fd_boot_split_peripheral_timer_t *timer = &fd_boot_split_peripheral.configuration.timer;
        timer->update(timer->context);
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

bool fd_boot_split_peripheral_io(fd_boot_error_t *error) {
    uint8_t buffer[fd_boot_split_peripheral_message_limit];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_boot_split_peripheral_timer_t *timer = &fd_boot_split_peripheral.configuration.timer;
    timer->initialize(timer->context);
    while (!fd_boot_split_peripheral_poll(&message)) {
        if (timer->has_timed_out(timer->context)) {
            timer->finalize(timer->context);
            fd_boot_set_error(error, 1);
            return false;
        }
    }
    timer->finalize(timer->context);

    fd_envelope_t envelope;
    bool result = fd_envelope_decode(&message, &envelope);
    if (!result) {
        fd_boot_set_error(error, 1);
        return false;
    }
    message.size = message.put_index;
    if (!fd_dispatch_process(&message, &envelope)) {
        fd_boot_set_error(error, 1);
        return false;
    }

    return true;
}

bool fd_boot_split_peripheral_tx(fd_binary_t *message, int type) {
    fd_envelope_t envelope = {
        .target = fd_boot_split_peripheral.configuration.target,
        .source = fd_boot_split_peripheral.configuration.source,
        .system = fd_boot_split_peripheral.configuration.system,
        .subsystem = fd_boot_split_peripheral.configuration.subsystem,
        .type = type,
    };
    if (!fd_envelope_encode(message, &envelope)) {
        return false;
    }
    fd_binary_put_uint8(message, 0);
    if (message->errors) {
        return false;
    }
    fd_boot_split_peripheral.configuration.transmit(message->buffer, message->put_index);
    return true;
}

void fd_boot_split_peripheral_status_progress(float amount) {
#if 0
    uint8_t buffer[32];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_binary_put_uint16(&message, fd_boot_split_operation_status_progress);
    fd_binary_put_float32(&message, amount);
    fd_boot_split_peripheral_tx(&message, fd_envelope_type_event);
#endif
}

bool fd_boot_split_peripheral_get_update_storage_request(void) {
    uint8_t buffer[32];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_binary_put_uint16(&message, fd_boot_split_operation_get_update_storage);
    return fd_boot_split_peripheral_tx(&message, fd_envelope_type_request);
}

bool fd_boot_split_peripheral_get_update_storage(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error) {
    fd_boot_split_peripheral_get_update_storage_operation_t get_update_storage_operation = {
        .storage = storage,
    };
    fd_boot_split_peripheral.operation.get_update_storage = &get_update_storage_operation;

    if (!fd_boot_split_peripheral_get_update_storage_request()) {
        fd_boot_set_error(error, 1);
        return false;
    }

    // beware: this is a nested dispatch loop... -denis
    while (fd_boot_split_peripheral.operation.any != 0) {
        if (!fd_boot_split_peripheral_io(error)) {
            return false;
        }
    }

    return true;
}

bool fd_boot_split_peripheral_get_update_storage_response(fd_binary_t *message) {
    if (fd_boot_split_peripheral.operation.get_update_storage == 0) {
        // response received with no pending operation
        return true;
    }

    fd_boot_split_peripheral.operation.get_update_storage->storage->range.location = fd_binary_get_uint32(message);
    fd_boot_split_peripheral.operation.get_update_storage->storage->range.length = fd_binary_get_uint32(message);
    fd_boot_split_peripheral.operation.get_update_storage = 0;
    return true;
}

bool fd_boot_split_peripheral_update_read_request(uint32_t location, uint32_t length) {
    uint8_t buffer[32];
    fd_binary_t message;
    fd_binary_initialize(&message, buffer, sizeof(buffer));
    fd_binary_put_uint16(&message, fd_boot_split_operation_update_read);
    fd_binary_put_uint32(&message, location);
    fd_binary_put_uint16(&message, (uint16_t)length);
    return fd_boot_split_peripheral_tx(&message, fd_envelope_type_request);
}

bool fd_boot_split_peripheral_update_read(
    void *context fd_unused,
    uint32_t location,
    uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
) {
    fd_boot_split_peripheral_update_read_operation_t update_read_operation = {
        .location = location,
        .data = data,
        .length = length,
        .error = error,
        .result = false,
    };
    fd_boot_split_peripheral.operation.update_read = &update_read_operation;

    if (!fd_boot_split_peripheral_update_read_request(location, length)) {
        fd_boot_set_error(error, 1);
        return false;
    }

    // beware: this is a nested dispatch loop... -denis
    while (fd_boot_split_peripheral.operation.any != 0) {
        if (!fd_boot_split_peripheral_io(error)) {
            return false;
        }
    }

    return update_read_operation.result;
}

bool fd_boot_split_peripheral_update_read_response(fd_binary_t *message) {
    if (fd_boot_split_peripheral.operation.update_read == 0) {
        // response received with no pending operation
        return true;
    }

    uint32_t location = fd_binary_get_uint32(message);
    if (location != fd_boot_split_peripheral.operation.update_read->location) {
        fd_boot_set_error(fd_boot_split_peripheral.operation.update_read->error, 1);
        fd_boot_split_peripheral.operation.update_read->result = false;
        return true;
    }
    uint32_t length = fd_binary_get_uint16(message);
    if (length != fd_boot_split_peripheral.operation.update_read->length) {
        fd_boot_set_error(fd_boot_split_peripheral.operation.update_read->error, 1);
        fd_boot_split_peripheral.operation.update_read->result = false;
        return true;
    }
    bool result = fd_binary_get_uint8(message) != 0;
    if (!result) {
        uint32_t code = fd_binary_get_uint32(message);
        fd_boot_set_error(fd_boot_split_peripheral.operation.update_read->error, code);
        fd_boot_split_peripheral.operation.update_read->result = false;
        return true;
    }
    if ((message->size - message->get_index) != length) {
        fd_boot_set_error(fd_boot_split_peripheral.operation.update_read->error, 1);
        fd_boot_split_peripheral.operation.update_read->result = false;
        return true;
    }
    memcpy(fd_boot_split_peripheral.operation.update_read->data, &message->buffer[message->get_index], length);
    fd_boot_split_peripheral.operation.update_read->result = true;
    fd_boot_split_peripheral.operation.update_read = 0;
    return true;
}

bool fd_boot_split_peripheral_dispatch_respond(fd_binary_t *message, fd_envelope_t *envelope) {
    if (!fd_envelope_encode(message, envelope)) {
        return false;
    }
    fd_binary_put_uint8(message, 0);
    if (message->errors) {
        return false;
    }
    fd_boot_split_peripheral.configuration.transmit(message->buffer, message->put_index);
    return true;
}

bool fd_boot_split_peripheral_get_identity(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_split_operation_get_identity);
    fd_system_identity_t *identity = &fd_boot_split_peripheral.configuration.identity;
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

bool fd_boot_split_peripheral_get_executable_metadata(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_executable_metadata_result_t result = {};
    bool success = fd_boot_get_executable_metadata(&fd_boot_split_peripheral.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_split_operation_get_executable_metadata);
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

bool fd_boot_split_peripheral_get_update_metadata(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_update_metadata_result_t result = {};
    bool success = fd_boot_get_update_metadata(&fd_boot_split_peripheral.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_split_operation_get_update_metadata);
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

bool fd_boot_split_peripheral_update(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_update_result_t result = {};
    bool success = fd_boot_update(&fd_boot_split_peripheral.configuration.update_interface, &result, &error);

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_split_operation_update);
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

bool fd_boot_split_peripheral_execute(fd_binary_t *message fd_unused, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    fd_boot_error_t error = {};
    fd_boot_get_executable_metadata_result_t result = {};
    bool success = fd_boot_get_executable_metadata(&fd_boot_split_peripheral.configuration.update_interface, &result, &error);
    if (success && result.is_valid) {
        // this call won't return of the execute is successful
        success = fd_boot_execute(&fd_boot_split_peripheral.configuration.update_interface, &error);
    }

    uint8_t data[64];
    fd_binary_t response;
    fd_binary_initialize(&response, data, sizeof(data));
    fd_binary_put_uint16(&response, fd_boot_split_operation_execute);
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

bool fd_boot_split_peripheral_dispatch_request(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint16_t operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_split_operation_get_identity:
            return fd_boot_split_peripheral_get_identity(message, envelope, respond);
        case fd_boot_split_operation_get_executable_metadata:
            return fd_boot_split_peripheral_get_executable_metadata(message, envelope, respond);
        case fd_boot_split_operation_get_update_metadata:
            return fd_boot_split_peripheral_get_update_metadata(message, envelope, respond);
        case fd_boot_split_operation_update:
            return fd_boot_split_peripheral_update(message, envelope, respond);
        case fd_boot_split_operation_execute:
            return fd_boot_split_peripheral_execute(message, envelope, respond);
        default:
            return false;
    }
}

bool fd_boot_split_peripheral_dispatch_response(fd_binary_t *message, fd_envelope_t *envelope fd_unused, fd_dispatch_respond_t respond fd_unused) {
    uint16_t operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_split_operation_get_update_storage:
            return fd_boot_split_peripheral_get_update_storage_response(message);
        case fd_boot_split_operation_update_read:
            return fd_boot_split_peripheral_update_read_response(message);
        default:
            return false;
    }
}

bool fd_boot_split_peripheral_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    if (envelope->type == fd_envelope_type_request) {
        return fd_boot_split_peripheral_dispatch_request(message, envelope, respond);
    }
    if (envelope->type == fd_envelope_type_response) {
        return fd_boot_split_peripheral_dispatch_response(message, envelope, respond);
    }
    return false;
}

void fd_boot_split_peripheral_timer_initialize(void *context) {
    fd_boot_split_peripheral.duration = 0;
}

void fd_boot_split_peripheral_timer_update(void *context) {
    fd_boot_split_peripheral.duration = 0;
}

bool fd_boot_split_peripheral_timer_has_timed_out(void *context) {
    if (fd_boot_split_peripheral.duration > fd_boot_split_peripheral_timeout) {
        return true;
    }
    ++fd_boot_split_peripheral.duration;
    return false;
}

void fd_boot_split_peripheral_timer_finalize(void *context) {
    fd_boot_split_peripheral.duration = 0;
}

bool fd_boot_split_peripheral_set_configuration_defaults(fd_boot_split_peripheral_configuration_t *configuration, fd_boot_error_t *error) {
    if (
        (configuration->target == 0) &&
        (configuration->source == 0) &&
        (configuration->system == 0) &&
        (configuration->subsystem == 0)
    ) {
        configuration->target = 1;
        configuration->source = 0;
        configuration->system = 0;
        configuration->subsystem = 0;
    }

    if (configuration->identity.identifier == 0) {
        configuration->identity.identifier = "fd_boot";
    }
    if (
        (configuration->identity.version.major == 0) &&
        (configuration->identity.version.minor == 0) &&
        (configuration->identity.version.patch == 0)
    ) {
        configuration->identity.version.major = 1;
        configuration->identity.version.minor = 0;
        configuration->identity.version.patch = 0;
    }

    if (configuration->timer.has_timed_out == 0) {
        configuration->timer.initialize = fd_boot_split_peripheral_timer_initialize;
        configuration->timer.update = fd_boot_split_peripheral_timer_update;
        configuration->timer.has_timed_out = fd_boot_split_peripheral_timer_has_timed_out;
        configuration->timer.finalize = fd_boot_split_peripheral_timer_finalize;
    }
    
    configuration->update_interface.info.get_update_storage = fd_boot_split_peripheral_get_update_storage;
    configuration->update_interface.status.progress = fd_boot_split_peripheral_status_progress;
    configuration->update_interface.update_reader.read = fd_boot_split_peripheral_update_read;

    return fd_boot_set_update_interface_defaults(&configuration->update_interface, error);
}

bool fd_boot_split_peripheral_dispatch_filter(const fd_envelope_t *envelope) {
    return
        (envelope->system == fd_boot_split_peripheral.configuration.system) &&
        (envelope->subsystem == fd_boot_split_peripheral.configuration.subsystem);
}

bool fd_boot_split_peripheral_start(fd_boot_split_peripheral_configuration_t *configuration, fd_boot_error_t *error) {
    memset(&fd_boot_split_peripheral, 0, sizeof(fd_boot_split_peripheral));
    fd_boot_split_peripheral.configuration = *configuration;
    fd_fifo_initialize(&fd_boot_split_peripheral.fifo, fd_boot_split_peripheral.fifo_buffer, sizeof(fd_boot_split_peripheral.fifo_buffer));

    fd_dispatch_initialize(fd_boot_split_peripheral_dispatch_respond);
    fd_dispatch_add_process(fd_boot_split_peripheral_dispatch_process, fd_boot_split_peripheral_dispatch_filter);

    while (true) {
        if (!fd_boot_split_peripheral_io(error)) {
            if (configuration->return_on_error) {
                return false;
            }
        }
    }

    fd_boot_set_error(error, 1);
    return false;
}
