#include "fd_spim_dispatch.h"

#include "fd_dispatch.h"
#include "fd_spim.h"

#ifndef fd_spim_dispatch_transfer_limit
#define fd_spim_dispatch_transfer_limit 8
#endif

#ifndef fd_spim_dispatch_rx_limit
#define fd_spim_dispatch_rx_limit 200
#endif

typedef enum {
    fd_spim_dispatch_operation_io,
} fd_spim_dispatch_operation_t;

bool fd_spim_dispatch_io(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    // decode the request
    uint32_t device_identifier = fd_binary_get_uint8(message);
    const fd_spim_device_t *device = fd_spim_get_device(device_identifier);
    if (device == 0) {
        return false;
    }

    fd_spim_transfer_t transfers[fd_spim_dispatch_transfer_limit];
    uint32_t transfer_count = fd_binary_get_uint8(message);
    if (transfer_count > fd_spim_dispatch_transfer_limit) {
        return false;
    }

    uint8_t rx[fd_spim_dispatch_rx_limit] = {
        fd_spim_dispatch_operation_io,
        device_identifier,
        transfer_count,
    };
    uint32_t rx_index = 3;
    for (uint32_t i = 0; i < transfer_count; ++i) {
        fd_spim_transfer_t *transfer = &transfers[i];
        transfer->tx_byte_count = fd_binary_get_uint8(message);
        if (!fd_binary_get_check(message, transfer->tx_byte_count)) {
            return false;
        }
        rx[rx_index++] = transfer->tx_byte_count;
        transfer->tx_bytes = &message->buffer[message->get_index];
        message->get_index += transfer->tx_byte_count;

        transfer->rx_byte_count = fd_binary_get_uint8(message);
        if ((rx_index + 1 + transfer->rx_byte_count) >= sizeof(rx)) {
            return false;
        }
        rx[rx_index++] = transfer->rx_byte_count;
        transfer->rx_bytes = &rx[rx_index];
        rx_index += transfer->rx_byte_count;
    }
    if (message->errors) {
        return false;
    }

    // process the request
    fd_spim_io_t io = {
        .transfer_count = transfer_count,
        .transfers = transfers,
    };
    fd_spim_device_io(device, &io);

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

bool fd_spim_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t operation = fd_binary_get_uint8(message);
    switch (operation) {
        case fd_spim_dispatch_operation_io:
            return fd_spim_dispatch_io(message, envelope, respond);
        break;
        default:
        return false;
    }
}

void fd_spim_dispatch_initialize(void) {
    fd_dispatch_add_process(fd_envelope_system_firefly, fd_envelope_subsystem_spim, fd_spim_dispatch_process);
}