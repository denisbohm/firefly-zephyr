#include "fd_i2cm_dispatch.h"

#include "fd_dispatch.h"
#include "fd_i2cm.h"

#ifndef fd_i2cm_dispatch_transfer_limit
#define fd_i2cm_dispatch_transfer_limit 8
#endif

#ifndef fd_i2cm_dispatch_rx_limit
#define fd_i2cm_dispatch_rx_limit 300
#endif

typedef enum {
    fd_i2cm_dispatch_operation_io,
} fd_i2cm_dispatch_operation_t;

bool fd_i2cm_dispatch_io(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    // decode the request
    uint32_t device_identifier = fd_binary_get_uint8(message);
    const fd_i2cm_device_t *device = fd_i2cm_get_device(device_identifier);
    if (device == 0) {
        return false;
    }

    fd_i2cm_transfer_t transfers[fd_i2cm_dispatch_transfer_limit];
    uint32_t transfer_count = fd_binary_get_uint8(message);
    if (transfer_count > fd_i2cm_dispatch_transfer_limit) {
        return false;
    }

    uint8_t rx[fd_i2cm_dispatch_rx_limit] = { fd_i2cm_dispatch_operation_io };
    uint32_t rx_index = 1;
    for (uint32_t i = 0; i < transfer_count; ++i) {
        fd_i2cm_transfer_t *transfer = &transfers[i];
        transfer->direction = fd_binary_get_uint8(message);
        transfer->byte_count = fd_binary_get_uint8(message);
        switch (transfer->direction) {
            case fd_i2cm_direction_tx:
                fd_binary_get_check(message, transfer->byte_count);
                transfer->bytes = &message->buffer[message->get_index];
                message->get_index += transfer->byte_count;
                break;
            case fd_i2cm_direction_rx:
                transfer->bytes = &rx[rx_index];
                rx_index += transfer->byte_count;
                if (rx_index > fd_i2cm_dispatch_rx_limit) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    if (message->errors) {
        return false;
    }

    // process the request
    fd_i2cm_io_t io = {
        .transfer_count = transfer_count,
        .transfers = transfers,
    };
    bool ack = fd_i2cm_device_io(device, &io);
    if (!ack) {
        return false;
    }

    // create response and send it back...
    fd_binary_t response;
    fd_binary_initialize(&response, rx, sizeof(rx));
    response.put_index = rx_index;
    fd_envelope_t response_envelope = {
        .target = envelope->source,
        .source = envelope->target,
        .system = envelope->system,
        .subsystem = envelope->subsystem,
        .type = fd_envelope_type_response,
    };
    return respond(&response, &response_envelope);
}

bool fd_i2cm_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t operation = fd_binary_get_uint8(message);
    switch (operation) {
        case fd_i2cm_dispatch_operation_io:
            return fd_i2cm_dispatch_io(message, envelope, respond);
        break;
        default:
        return false;
    }
}

void fd_i2cm_dispatch_initialize(void) {
    fd_dispatch_add_process(fd_envelope_system_firefly, fd_envelope_subsystem_i2cm, fd_i2cm_dispatch_process);
}