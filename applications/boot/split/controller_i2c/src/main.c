#include "update.h"

#include "fd_boot_split.h"
#include "fd_boot_split_controller.h"
#include "fd_gpio.h"
#include "fd_i2cm.h"

#include <string.h>

typedef struct {
    const fd_i2cm_device_t *device;
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
    return fd_i2cm_device_txn(fd_boot_serial_controller.device, data, length);
}

void fd_boot_serial_controller_poll(void) {
    uint8_t data[32];
    bool result = fd_i2cm_device_rxn(fd_boot_serial_controller.device, data, sizeof(data));
    if (!result) {
        return;
    }
    uint32_t length = (uint32_t)data[0];
    if (length == 0) {
        return;
    }
    if (length >= sizeof(data)) {
        length = sizeof(data) - 1;
    }
    fd_boot_split_controller_received(&fd_boot_serial_controller.controller, &data[1], length);
}

int main(void) {
    fd_boot_serial_controller = (fd_boot_serial_controller_t) {
        .controller = {
            .target = 0,
            .source = 0,
            .system = 0,
            .subsystem = 0,
            .get_update_storage = fd_boot_serial_controller_get_update_storage,
            .update_read = fd_boot_serial_controller_update_read,
            .progress = fd_boot_serial_controller_progress,
            .transmit = fd_boot_serial_controller_transmit,
            .poll = fd_boot_serial_controller_poll,
        },
    };
    fd_boot_split_controller_t *controller = &fd_boot_serial_controller.controller;
    uint8_t fifo_data[128];
    fd_fifo_initialize(&controller->fifo, fifo_data, sizeof(fifo_data));

    fd_gpio_initialize();

    const static fd_i2cm_bus_t i2cm_buses[] = {
        {
            .i2c_device_name = "NRF_TWIM0_S", // 0 to use software I2C bit banging
            .scl = { .port = 1, .pin = 2 },
            .sda = { .port = 1, .pin = 3 },
            .pullup = false,
            .frequency = 100000,
        },
    };
    const static fd_i2cm_device_t i2cm_devices[] = {
        {
            .bus = &i2cm_buses[0],
            .address = 0x69,
        },
    };
    fd_boot_serial_controller.device = &i2cm_devices[0];
    fd_i2cm_initialize(i2cm_buses, 1, i2cm_devices, 1);
    fd_i2cm_bus_enable(&i2cm_buses[0]);

    fd_boot_error_t error;
    if (!fd_boot_split_controller_set_defaults(controller, &error)) {
        return 1;
    }
    if (!fd_boot_split_controller_initialize(controller, &error)) {
        return 2;
    }

    fd_version_t version;
    char identifier[32];
    if (!fd_boot_split_controller_get_identity(controller, &version, identifier, sizeof(identifier), &error)) {
        return 3;
    }

    fd_boot_split_controller_update_result_t update_result;
    if (!fd_boot_split_controller_update(controller, &update_result, &error)) {
        return 4;
    }
    if (!update_result.is_valid) {
        return 5;
    }

    fd_boot_split_controller_execute_result_t execute_result;
    if (!fd_boot_split_controller_execute(controller, &execute_result, &error)) {
        // this should time out, because the boot loader disabled the peripheral and the application firmware was started
        return 0;
    }
    if (!update_result.is_valid) {
        return 6;
    }
    
    return 7;
}
