#include "update.h"

#include "fd_boot_split.h"
#include "fd_boot_split_controller.h"
#include "fd_uart.h"

#include <string.h>

typedef struct {
    fd_uart_instance_t uart_instance;
} fd_boot_serial_controller_t;

fd_boot_serial_controller_t fd_boot_serial_controller;

void fd_boot_serial_controller_get_update_storage(fd_boot_range_t *range) {
    range->location = 0;
    range->length = update_length;
}

void fd_boot_serial_controller_update_read(uint32_t location, uint8_t *data, uint32_t length) {
    memcpy(data, &update_data[location], length);
}

void fd_boot_serial_controller_progress(float amount) {
}

bool fd_boot_serial_controller_transmit(const uint8_t *data, uint32_t length) {
    uint32_t actual = fd_uart_instance_tx(&fd_boot_serial_controller.uart_instance, data, length);
    return actual == length;
}

int main(void) {
    memset(&fd_boot_serial_controller, 0, sizeof(fd_boot_serial_controller));
    fd_boot_serial_controller.uart_instance = (fd_uart_instance_t) {
        .uart_device_name = "NRF_UARTE3_S",
        .tx_gpio = { .port = 0, .pin = 0 },
        .rx_gpio = { .port = 0, .pin = 0 },
        .baud_rate = 115200,
        .parity = fd_uart_parity_none,
        .stop_bits = fd_uart_stop_bits_1,
        .tx_event_name = "fd_uart_tx",
        .rx_event_name = "fd_uart.rx",
    };

    fd_uart_initialize();
    fd_uart_instance_initialize(&fd_boot_serial_controller.uart_instance);

    fd_boot_error_t error;
    fd_boot_split_controller_t controller = {
        .get_update_storage = fd_boot_serial_controller_get_update_storage,
        .update_read = fd_boot_serial_controller_update_read,
        .progress = fd_boot_serial_controller_progress,
        .transmit = fd_boot_serial_controller_transmit,
    };
    if (!fd_boot_split_controller_initialize(&controller, &error)) {
        return false;
    }

    fd_version_t version;
    char identifier[32];
    if (!fd_boot_split_controller_get_identity(&controller, &version, identifier, sizeof(identifier), &error)) {
        return false;
    }

    fd_boot_split_controller_update_result_t update_result;
    if (!fd_boot_split_controller_update(&controller, &update_result, &error)) {
        return false;
    }
    if (!update_result.is_valid) {
        return false;
    }

    fd_boot_split_controller_execute_result_t execute_result;
    if (!fd_boot_split_controller_execute(&controller, &execute_result, &error)) {

    }
    if (!update_result.is_valid) {
        return false;
    }
    
    return 0;
}
