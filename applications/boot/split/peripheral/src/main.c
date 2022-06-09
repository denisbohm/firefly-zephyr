#include "fd_boot_nrf53.h"
#include "fd_boot_split_peripheral.h"
#include "fd_gpio.h"
#include "fd_uart.h"

#include <string.h>

typedef struct {
    fd_uart_instance_t uart_instance;
} fd_boot_serial_peripheral_t;

fd_boot_serial_peripheral_t fd_boot_serial_peripheral;

#define FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_STORAGE_LOCATION 0x80000
#define FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_STORAGE_LENGTH (32 * 1024)

#define FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_LOCATION 0x80000
#define FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_METADATA_OFFSET 0x180

#define FD_BOOT_SERIAL_PERIPHERAL_DECRYPTION_KEY 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f

bool fd_boot_serial_peripheral_get_executable_storage(
    fd_boot_info_executable_storage_t *storage,
    fd_boot_error_t *error
) {
    *storage = (fd_boot_info_executable_storage_t) {
        .range = {
            .location = FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_STORAGE_LOCATION,
            .length = FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_STORAGE_LENGTH,
        },
    };
    return true;
}

bool fd_boot_serial_peripheral_get_executable(
    fd_boot_info_executable_t *executable,
    fd_boot_error_t *error
) {
    *executable = (fd_boot_info_executable_t) {
        .location = FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_LOCATION,
        .metadata_offset = FD_BOOT_SERIAL_PERIPHERAL_EXECUTABLE_METADATA_OFFSET,
    };
    return true;
}

bool fd_boot_serial_peripheral_get_decryption(
    fd_boot_info_decryption_t *decryption,
    fd_boot_error_t *error
) {
    const uint8_t key[] = { FD_BOOT_SERIAL_PERIPHERAL_DECRYPTION_KEY };
    memcpy(decryption->key.data, key, sizeof(key));
    return true;
}

void fd_boot_serial_peripheral_transmit(const uint8_t *data, uint32_t length) {
    fd_uart_instance_tx(&fd_boot_serial_peripheral.uart_instance, data, length);
}

void fd_boot_serial_peripheral_isr_tx(void *context) {
}

void fd_boot_serial_peripheral_isr_rx(void *context) {
    uint8_t data[32];
    uint32_t length;
    while (true) {
        length = fd_uart_instance_rx(&fd_boot_serial_peripheral.uart_instance, data, sizeof(data));
        if (length == 0) {
            break;
        }
        fd_boot_split_peripheral_received(data, length);
    }
}

int main(void) {
    memset(&fd_boot_serial_peripheral, 0, sizeof(fd_boot_serial_peripheral));
    fd_boot_serial_peripheral.uart_instance = (fd_uart_instance_t) {
        .uart_device_name = "NRF_UARTE0_S",
        .tx_gpio = { .port = 0, .pin = 20 },
        .rx_gpio = { .port = 0, .pin = 22 },
        .baud_rate = 115200,
        .parity = fd_uart_parity_none,
        .stop_bits = fd_uart_stop_bits_1,
        .isr_tx_callback = fd_boot_serial_peripheral_isr_tx,
        .isr_rx_callback = fd_boot_serial_peripheral_isr_rx,
    };

    fd_boot_error_t error;
    fd_boot_split_peripheral_configuration_t configuration = {
        .transmit = fd_boot_serial_peripheral_transmit,
        .update_interface = {
            .info = {
                .get_executable_storage = fd_boot_serial_peripheral_get_executable_storage,
                .get_executable = fd_boot_serial_peripheral_get_executable,
                .get_decryption = fd_boot_serial_peripheral_get_decryption,
            },
            .executable_flasher = {
                .context = 0,
                .erase = fd_boot_nrf53_flasher_erase,
                .write = fd_boot_nrf53_flasher_write,
                .finalize = fd_boot_nrf53_flasher_finalize,
            },
            .executor = {
                .cleanup = fd_boot_nrf53_executor_cleanup,
                .start = fd_boot_nrf53_executor_start,
            },
        },
    };
    if (!fd_boot_split_peripheral_set_configuration_defaults(&configuration, &error)) {
        return 1;
    }

    // start the executable if it is valid
    fd_boot_get_executable_metadata_result_t executable;
    if (!fd_boot_get_executable_metadata(&configuration.update_interface, &executable, &error)) {
        return 2;
    }
    if (executable.is_valid) {
        if (!fd_boot_execute(&configuration.update_interface, &error)) {
            return 3;
        }
    }

    // start the boot loader serial interface
    fd_gpio_initialize();
    fd_uart_initialize();
    fd_uart_instance_initialize(&fd_boot_serial_peripheral.uart_instance);
    fd_boot_split_peripheral_initialize(&configuration);
    if (!fd_boot_split_peripheral_run(&error)) {
        return 4;
    }
    
    return 0;
}
