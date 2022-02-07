#include "update.h"

#include "fd_boot_split.h"
#include "fd_boot_split_controller.h"
#include "fd_gpio.h"
#include "fd_uart.h"

#include <string.h>

typedef struct {
    fd_uart_instance_t uart_instance;
    fd_boot_split_controller_t controller;
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

void fd_boot_serial_controller_isr_tx(void) {
}

void fd_boot_serial_controller_isr_rx(void) {
    uint8_t data[32];
    uint32_t length;
    while (true) {
        length = fd_uart_instance_rx(&fd_boot_serial_controller.uart_instance, data, sizeof(data));
        if (length == 0) {
            break;
        }
        fd_boot_split_controller_received(&fd_boot_serial_controller.controller, data, length);
    }
}

int main(void) {
    memset(&fd_boot_serial_controller, 0, sizeof(fd_boot_serial_controller));
    fd_boot_serial_controller.uart_instance = (fd_uart_instance_t) {
        .uart_device_name = "NRF_UARTE0_S",
        .tx_gpio = { .port = 0, .pin = 20 },
        .rx_gpio = { .port = 0, .pin = 22 },
        .baud_rate = 115200,
        .parity = fd_uart_parity_none,
        .stop_bits = fd_uart_stop_bits_1,
        .isr_tx_callback = fd_boot_serial_controller_isr_tx,
        .isr_rx_callback = fd_boot_serial_controller_isr_rx,
    };
    fd_boot_serial_controller.controller = (fd_boot_split_controller_t) {
        .target = 0,
        .source = 0,
        .system = 0,
        .subsystem = 0,
        .get_update_storage = fd_boot_serial_controller_get_update_storage,
        .update_read = fd_boot_serial_controller_update_read,
        .progress = fd_boot_serial_controller_progress,
        .transmit = fd_boot_serial_controller_transmit,
    };
    uint8_t fifo_data[128];
    fd_fifo_initialize(&fd_boot_serial_controller.controller.fifo, fifo_data, sizeof(fifo_data));

    fd_gpio_initialize();
    fd_uart_initialize();
    fd_uart_instance_initialize(&fd_boot_serial_controller.uart_instance);

    fd_boot_error_t error;
    fd_boot_split_controller_t *controller = &fd_boot_serial_controller.controller;
    if (!fd_boot_split_controller_initialize(controller, &error)) {
        return 1;
    }

    fd_version_t version;
    char identifier[32];
    if (!fd_boot_split_controller_get_identity(controller, &version, identifier, sizeof(identifier), &error)) {
        return 2;
    }

    fd_boot_split_controller_update_result_t update_result;
    if (!fd_boot_split_controller_update(controller, &update_result, &error)) {
        return 3;
    }
    if (!update_result.is_valid) {
        return 4;
    }

    fd_boot_split_controller_execute_result_t execute_result;
    if (!fd_boot_split_controller_execute(controller, &execute_result, &error)) {
        // this should time out, because the boot loader disabled the uart and the application firmware was started
        return 0;
    }
    if (!update_result.is_valid) {
        return 6;
    }
    
    return 7;
}
