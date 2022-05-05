#include "fd_boot_split_controller.h"

#include "fd_boot_split.h"
#include "fd_envelope.h"

#include <string.h>

#ifndef fd_boot_split_controller_message_limit
#define fd_boot_split_controller_message_limit 300
#endif

#ifndef fd_boot_split_controller_timeout
#define fd_boot_split_controller_timeout 100000
#endif

bool fd_boot_split_controller_transmit(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    if (!fd_envelope_encode(message, envelope)) {
        return false;
    }
    fd_binary_put_uint8(message, 0);
    if (message->errors) {
        return false;
    }
    return controller->transmit(message->buffer, message->put_index);
}

void fd_boot_split_controller_get_update_storage(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    fd_boot_range_t range;
    controller->get_update_storage(&range);
    uint8_t response_buffer[64];
    fd_binary_t response;
    fd_binary_initialize(&response, response_buffer, sizeof(response_buffer));
    fd_binary_put_uint16(&response, fd_boot_split_operation_get_update_storage);
    fd_binary_put_uint32(&response, range.location);
    fd_binary_put_uint32(&response, range.length);
    fd_envelope_t response_envelope = {
        .target=controller->target,
        .source=controller->source,
        .system=controller->system,
        .subsystem=controller->subsystem,
        .type=fd_envelope_type_response,
    };
    fd_boot_split_controller_transmit(controller, &response, &response_envelope);
}

void fd_boot_split_controller_update_read(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    int location = fd_binary_get_uint32(message);
    int length = fd_binary_get_uint16(message);

    uint8_t response_buffer[fd_boot_split_controller_message_limit];
    fd_binary_t response;
    fd_binary_initialize(&response, response_buffer, sizeof(response_buffer));
    fd_binary_put_uint16(&response, fd_boot_split_operation_update_read);
    fd_binary_put_uint32(&response, location);
    fd_binary_put_uint16(&response, length);
    fd_boot_range_t range;
    controller->get_update_storage(&range);
    if ((location + length) < (range.location + range.length)) { // !!! check response buffer size also...
        bool result = true;
        fd_binary_put_uint8(&response, result ? 1 : 0);
        controller->update_read(location, &response.buffer[response.put_index], length);
        response.put_index += length;
    } else {
        bool result = false;
        fd_binary_put_uint8(&response, result ? 1 : 0);
    }
    fd_envelope_t response_envelope = {
        .target=controller->target,
        .source=controller->source,
        .system=controller->system,
        .subsystem=controller->subsystem,
        .type=fd_envelope_type_response,
    };
    fd_boot_split_controller_transmit(controller, &response, &response_envelope);
}

void fd_boot_split_controller_request(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    int operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_split_operation_get_update_storage:
            fd_boot_split_controller_get_update_storage(controller, message, envelope);
            break;
        case fd_boot_split_operation_update_read:
            fd_boot_split_controller_update_read(controller, message, envelope);
            break;
    }
}

void fd_boot_split_controller_status_progress(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    float amount = fd_binary_get_float32(message);
    controller->progress(amount);
}

void fd_boot_split_controller_event(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    int operation = fd_binary_get_uint16(message);
    switch (operation) {
        case fd_boot_split_operation_status_progress:
            fd_boot_split_controller_status_progress(controller, message, envelope);
            break;
    }
}

bool fd_boot_split_controller_dispatch(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    fd_envelope_t *envelope
) {
    fd_binary_t response;
    fd_binary_initialize(&response, message->buffer, message->put_index);
    switch (envelope->type) {
        case fd_envelope_type_request:
            fd_boot_split_controller_request(controller, &response, envelope);
            break;
        case fd_envelope_type_event:
            fd_boot_split_controller_event(controller, &response, envelope);
            break;
        case fd_envelope_type_response:
            return true;
    }
    return false;
}

bool fd_boot_split_controller_received(
    fd_boot_split_controller_t *controller,
    const uint8_t *data,
    uint32_t length
) {
    for (uint32_t i = 0; i < length; ++i) {
        fd_fifo_put(&controller->fifo, data[i]);
    }
    return true;
}

bool fd_boot_split_controller_poll(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message
) {
    uint8_t byte = 0;
    while (fd_fifo_get(&controller->fifo, &byte)) {
        fd_boot_split_controller_timer_t *timer = &controller->timer;
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

bool fd_boot_split_controller_io(
    fd_boot_split_controller_t *controller,
    fd_binary_t *message,
    bool *has_response,
    fd_boot_error_t *error
) {
    fd_boot_split_controller_timer_t *timer = &controller->timer;
    timer->initialize(timer->context);
    while (!fd_boot_split_controller_poll(controller, message)) {
        if (timer->has_timed_out(timer->context)) {
            timer->finalize(timer->context);
            fd_boot_set_error(error, 1);
            return false;
        }
        if (controller->poll) {
            controller->poll();
        }
    }
    timer->finalize(timer->context);

    fd_envelope_t envelope;
    bool result = fd_envelope_decode(message, &envelope);
    if (!result) {
        fd_boot_set_error(error, 1);
        return false;
    }
    *has_response = fd_boot_split_controller_dispatch(controller, message, &envelope);
    fd_binary_reset(message);
    return true;
}

bool fd_boot_split_controller_rpc(
    fd_boot_split_controller_t *controller,
    fd_boot_split_operation_t operation,
    fd_binary_t *request,
    fd_binary_t *response,
    fd_boot_error_t *error
) {
    fd_binary_put_uint16(request, operation);
    fd_envelope_t request_envelope = {
        .target = controller->target,
        .source = controller->source,
        .system = controller->system,
        .subsystem = controller->subsystem,
        .type = fd_envelope_type_request,
    };
    fd_boot_split_controller_transmit(controller, request, &request_envelope);
    while (true) {
        bool has_response = false;
        if (!fd_boot_split_controller_io(controller, response, &has_response, error)) {
            return false;
        }
        if (has_response) {
            break;
        }
    }
    uint16_t response_operation = fd_binary_get_uint16(response);
    if (response_operation != operation) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_split_controller_get_identity(
    fd_boot_split_controller_t *controller,
    fd_version_t *version,
    char *identifier,
    size_t identifier_size,
    fd_boot_error_t *error
) {
    uint8_t buffer[64];
    fd_binary_t request;
    fd_binary_initialize(&request, buffer, sizeof(buffer));
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    if (!fd_boot_split_controller_rpc(controller, fd_boot_split_operation_get_identity, &request, &response, error)) {
        return false;
    }

    version->major = fd_binary_get_uint32(&response);
    version->minor = fd_binary_get_uint32(&response);
    version->patch = fd_binary_get_uint32(&response);
    fd_binary_string_t string = fd_binary_get_string(&response);
    if ((string.length + 1) > identifier_size) {
        fd_boot_set_error(error, 1);
        return false;
    }
    memcpy(identifier, string.data, string.length);
    identifier[string.length] = '\0';
    return true;
}

bool fd_boot_split_controller_update(
    fd_boot_split_controller_t *controller,
    fd_boot_split_controller_update_result_t *result,
    fd_boot_error_t *error
) {
    uint8_t buffer[64];
    fd_binary_t request;
    fd_binary_initialize(&request, buffer, sizeof(buffer));
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    if (!fd_boot_split_controller_rpc(controller, fd_boot_split_operation_update, &request, &response, error)) {
        return false;
    }

    bool success = fd_binary_get_uint8(&response) != 0;
    if (!success) {
        uint32_t code = fd_binary_get_uint32(&response);
        fd_boot_set_error(error, code);
        return false;
    }
    result->is_valid = fd_binary_get_uint8(&response) != 0;
    if (!result->is_valid) {
        result->issue = fd_binary_get_uint8(&response);
    }
    return true;
}

bool fd_boot_split_controller_execute(
    fd_boot_split_controller_t *controller,
    fd_boot_split_controller_execute_result_t *result,
    fd_boot_error_t *error
) {
    uint8_t buffer[64];
    fd_binary_t request;
    fd_binary_initialize(&request, buffer, sizeof(buffer));
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    if (!fd_boot_split_controller_rpc(controller, fd_boot_split_operation_execute, &request, &response, error)) {
        return false;
    }

    bool success = fd_binary_get_uint8(&response) != 0;
    if (!success) {
        uint32_t code = fd_binary_get_uint32(&response);
        fd_boot_set_error(error, code);
        return false;
    }
    result->is_valid = fd_binary_get_uint8(&response) != 0;
    if (!result->is_valid) {
        result->issue = fd_binary_get_uint8(&response);
    }
    return true;
}

void fd_boot_split_controller_timer_initialize(void *context) {
    uint32_t *duration = (uint32_t *)context;
    *duration = 0;
}

void fd_boot_split_controller_timer_update(void *context) {
    uint32_t *duration = (uint32_t *)context;
    *duration = 0;
}

bool fd_boot_split_controller_timer_has_timed_out(void *context) {
    uint32_t *duration = (uint32_t *)context;
    if (*duration > fd_boot_split_controller_timeout) {
        return true;
    }
    ++*duration;
    return false;
}

void fd_boot_split_controller_timer_finalize(void *context) {
    uint32_t *duration = (uint32_t *)context;
    *duration = 0;
}

bool fd_boot_split_controller_set_defaults(
    fd_boot_split_controller_t *controller,
    fd_boot_error_t *error
) {
    if (controller->timer.has_timed_out == 0) {
        controller->timer.context = &controller->duration;
        controller->timer.initialize = fd_boot_split_controller_timer_initialize;
        controller->timer.update = fd_boot_split_controller_timer_update;
        controller->timer.has_timed_out = fd_boot_split_controller_timer_has_timed_out;
        controller->timer.finalize = fd_boot_split_controller_timer_finalize;
    }

    return true;
}

bool fd_boot_split_controller_initialize(
    fd_boot_split_controller_t *controller,
    fd_boot_error_t *error
) {
    return true;
}
