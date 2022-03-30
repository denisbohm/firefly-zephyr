#include "fd_gpio_dispatch.h"

#include "fd_dispatch.h"
#include "fd_gpio.h"

typedef enum {
    fd_gpio_dispatch_operation_io = 0,
} fd_gpio_dispatch_operation_t;

typedef enum {
    fd_gpio_dispatch_opcode_configure,
    fd_gpio_dispatch_opcode_set,
    fd_gpio_dispatch_opcode_get,
} fd_gpio_dispatch_opcode_t;

#define fd_gpio_direction_none   0b00
#define fd_gpio_direction_input  0b01
#define fd_gpio_direction_output 0b10

#define fd_gpio_pull_none 0b00
#define fd_gpio_pull_down 0b01
#define fd_gpio_pull_up   0b10

#define fd_gpio_level_low  0b0
#define fd_gpio_level_high 0b1

#define fd_gpio_drive_none 0b00
#define fd_gpio_drive_low  0b01
#define fd_gpio_drive_high 0b10
#define fd_gpio_drive_both 0b11

#define fd_gpio_dispatch_configure(direction, pull, level, drive) (((drive) << 5) | ((level) << 4) | ((pull) << 2) | (direction))

typedef enum {
    fd_gpio_dispatch_configure_default =
        fd_gpio_dispatch_configure(fd_gpio_direction_none, fd_gpio_pull_none, fd_gpio_level_low, fd_gpio_drive_none),
    fd_gpio_dispatch_configure_output_low =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_none, fd_gpio_level_low, fd_gpio_drive_both),
    fd_gpio_dispatch_configure_output_high =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_none, fd_gpio_level_high, fd_gpio_drive_both),
    fd_gpio_dispatch_configure_output_open_drain_low =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_none, fd_gpio_level_low, fd_gpio_drive_low),
    fd_gpio_dispatch_configure_output_open_drain_high =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_none, fd_gpio_level_high, fd_gpio_drive_low),
    fd_gpio_dispatch_configure_output_open_drain_pull_up_low =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_up, fd_gpio_level_low, fd_gpio_drive_low),
    fd_gpio_dispatch_configure_output_open_drain_pull_up_high =
        fd_gpio_dispatch_configure(fd_gpio_direction_output, fd_gpio_pull_up, fd_gpio_level_high, fd_gpio_drive_low),
    fd_gpio_dispatch_configure_input =
        fd_gpio_dispatch_configure(fd_gpio_direction_input, fd_gpio_pull_none, fd_gpio_level_low, fd_gpio_drive_none),
    fd_gpio_dispatch_configure_input_pull_up =
        fd_gpio_dispatch_configure(fd_gpio_direction_input, fd_gpio_pull_up, fd_gpio_level_low, fd_gpio_drive_none),
} fd_gpio_dispatch_configure_t;

#define fd_gpio_dispatch_response_limit 64

void fd_gpio_dispatch_io_configure(fd_binary_t *message) {
    uint8_t port = fd_binary_get_uint8(message);
    uint8_t pin = fd_binary_get_uint8(message);
    fd_gpio_t gpio = { .port = port, .pin = pin };
    fd_gpio_dispatch_configure_t configure = fd_binary_get_uint8(message);
    switch (configure) {
        case fd_gpio_dispatch_configure_default:
            fd_gpio_configure_default(gpio);
            break;
        case fd_gpio_dispatch_configure_output_low:
            fd_gpio_configure_output(gpio, false);
            break;
        case fd_gpio_dispatch_configure_output_high:
            fd_gpio_configure_output(gpio, true);
            break;
        case fd_gpio_dispatch_configure_output_open_drain_low:
            fd_gpio_configure_output_open_drain(gpio, false);
            break;
        case fd_gpio_dispatch_configure_output_open_drain_high:
            fd_gpio_configure_output_open_drain(gpio, true);
            break;
        case fd_gpio_dispatch_configure_output_open_drain_pull_up_low:
            fd_gpio_configure_output_open_drain_pull_up(gpio, false);
            break;
        case fd_gpio_dispatch_configure_output_open_drain_pull_up_high:
            fd_gpio_configure_output_open_drain_pull_up(gpio, true);
            break;
        case fd_gpio_dispatch_configure_input:
            fd_gpio_configure_input(gpio);
            break;
        case fd_gpio_dispatch_configure_input_pull_up:
            fd_gpio_configure_input_pull_up(gpio);
            break;
        default:
            break;
    }
}

void fd_gpio_dispatch_io_set(fd_binary_t *message) {
    uint8_t port = fd_binary_get_uint8(message);
    uint8_t pin = fd_binary_get_uint8(message);
    fd_gpio_t gpio = { .port = port, .pin = pin };
    bool value = fd_binary_get_uint8(message) != 0;
    fd_gpio_set(gpio, value);
}

void fd_gpio_dispatch_io_get(fd_binary_t *message, fd_binary_t *response) {
    uint8_t port = fd_binary_get_uint8(message);
    uint8_t pin = fd_binary_get_uint8(message);
    fd_gpio_t gpio = { .port = port, .pin = pin };
    bool value = fd_gpio_get(gpio);
    fd_binary_put_uint8(response, fd_gpio_dispatch_opcode_get);
    fd_binary_put_uint8(response, port);
    fd_binary_put_uint8(response, pin);
    fd_binary_put_uint8(response, value ? 1 : 0);
}

bool fd_gpio_dispatch_io(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t response_buffer[fd_gpio_dispatch_response_limit];
    fd_binary_t response;
    fd_binary_initialize(&response, response_buffer, sizeof(response_buffer));
    fd_binary_put_uint8(&response, fd_gpio_dispatch_operation_io);
    while (message->get_index < message->size) {
        uint8_t opcode = fd_binary_get_uint8(message);
        switch (opcode) {
            case fd_gpio_dispatch_opcode_configure: {
                fd_gpio_dispatch_io_configure(message);
            } break;
            case fd_gpio_dispatch_opcode_set: {
                fd_gpio_dispatch_io_set(message);
            } break;
            case fd_gpio_dispatch_opcode_get: {
                fd_gpio_dispatch_io_get(message, &response);
            } break;
            default:
                break;
        }
    }
    if (message->errors) {
        return false;
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

bool fd_gpio_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t operation = fd_binary_get_uint8(message);
    switch (operation) {
        case fd_gpio_dispatch_operation_io:
            return fd_gpio_dispatch_io(message, envelope, respond);
        break;
        default:
            return false;
    }
}

void fd_gpio_dispatch_initialize(void) {
    fd_dispatch_add_process(fd_envelope_system_firefly, fd_envelope_subsystem_gpio, fd_gpio_dispatch_process);
}