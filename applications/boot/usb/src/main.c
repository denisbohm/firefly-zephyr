#include "fd_boot.h"
#include "fd_boot_nrf53.h"
#include "fd_boot_zephyr.h"
#include "fd_storage_fatfs.h"
#include "fd_usb.h"
#include "fd_usb_msc.h"

#include <zephyr.h>

#include <nrf5340_application.h>

#include <hal/nrf_gpio.h>

#include <string.h>

#define FD_BOOT_EXECUTABLE_STORAGE_LOCATION 0x80000
#define FD_BOOT_EXECUTABLE_STORAGE_LENGTH (32 * 1024)

#define FD_BOOT_EXECUTABLE_LOCATION 0x80000
#define FD_BOOT_EXECUTABLE_METADATA_OFFSET 0x180

#define FD_BOOT_DECRYPTION_KEY 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f

bool fd_boot_zephyr_get_executable_storage(fd_boot_info_executable_storage_t *storage, fd_boot_error_t *error) {
    *storage = (fd_boot_info_executable_storage_t) {
        .range = {
            .location = FD_BOOT_EXECUTABLE_STORAGE_LOCATION,
            .length = FD_BOOT_EXECUTABLE_STORAGE_LENGTH,
        },
    };
    return true;
}

bool fd_boot_zephyr_get_executable(fd_boot_info_executable_t *executable, fd_boot_error_t *error) {
    *executable = (fd_boot_info_executable_t) {
        .location = FD_BOOT_EXECUTABLE_LOCATION,
        .metadata_offset = FD_BOOT_EXECUTABLE_METADATA_OFFSET,
    };
    return true;
}

bool fd_boot_zephyr_get_decryption(fd_boot_info_decryption_t *decryption, fd_boot_error_t *error) {
    const uint8_t key[] = { FD_BOOT_DECRYPTION_KEY };
    memcpy(decryption->key.data, key, FD_BOOT_CRYPTO_KEY_SIZE);
    return true;
}

void fd_boot_zephyr_feed(void) {
}

bool fd_boot_zephyr_can_upgrade(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version) {
    return true;
}

bool fd_boot_zephyr_can_downgrade(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version) {
    return false;
}

bool fd_boot_zephyr_can_install(const fd_boot_version_t *version) {
    return true;
}

void fd_boot_zephyr_progress(float amount) {
}

void fd_boot_usb_start(void) {
    fd_usb_msc_initialize();
    fd_usb_initialize();
}

fd_boot_update_interface_t fd_boot_update_interface = {
    .info = {
        .get_executable_storage = fd_boot_zephyr_get_executable_storage,
        .get_executable = fd_boot_zephyr_get_executable,
        .get_update_storage = fd_boot_zephyr_get_update_storage,
        .get_decryption = fd_boot_zephyr_get_decryption,
    },
    .status = {
        .progress = fd_boot_zephyr_progress,
    },
    .watchdog = {
        .feed = fd_boot_zephyr_feed,
    },
    .hash = {
        .initialize = fd_boot_zephyr_hash_initialize,
        .update = fd_boot_zephyr_hash_update,
        .finalize = fd_boot_zephyr_hash_finalize,
    },
    .decrypt = {
        .initialize = fd_boot_zephyr_decrypt_initialize,
        .update = fd_boot_zephyr_decrypt_update,
        .finalize = fd_boot_zephyr_decrypt_finalize,
    },
    .executable_reader = {
        .context = 0,
        .read = fd_boot_zephyr_executable_read,
    },
    .executable_flasher = {
        .context = 0,
        .erase = fd_boot_nrf53_flasher_erase,
        .write = fd_boot_nrf53_flasher_write,
        .finalize = fd_boot_nrf53_flasher_finalize,
    },
    .update_reader = {
        .context = 0,
        .read = fd_boot_zephyr_update_read,
    },
    .action = {
        .can_upgrade = fd_boot_zephyr_can_upgrade,
        .can_downgrade = fd_boot_zephyr_can_downgrade,
        .can_install = fd_boot_zephyr_can_install,
    },
    .executor = {
        .cleanup = fd_boot_zephyr_executor_cleanup,
        .start = fd_boot_nrf53_executor_start,
    },
};

void fd_boot_try_execute(void) {
    fd_boot_error_t error;
    fd_boot_get_executable_metadata_result_t executable;
    if (!fd_boot_get_executable_metadata(&fd_boot_update_interface, &executable, &error)) {
        return;
    }
    if (!fd_boot_execute(&fd_boot_update_interface, &error)) {
        // cant execute
    }
}

void fd_boot_try_update(void) {
    fd_boot_error_t error;
    if (!fd_boot_zephyr_update_storage_open("NAND:/update.bin", &error)) {
        fd_boot_try_execute();
        return;
    }

    fd_boot_update_result_t result;
    if (!fd_boot_update(&fd_boot_update_interface, &result, &error)) {
        // printf("error %d\n", error.code);
    } else {
        if (result.is_valid) {
            // printf("is valid\n");
            if (!fd_boot_execute(&fd_boot_update_interface, &error)) {
                // cant execute
            }
        } else {
            // printf("issue %d\n", result.issue);
        }
    }
}

int main(void) {
    fd_storage_fatfs_initialize();
    if (!fd_storage_fatfs_open()) {
        fd_boot_try_execute();
        return 1;
    }
    if (fd_storage_fatfs_mount()) {
        const uint32_t button_1_pin = 23;
        nrf_gpio_cfg_input(button_1_pin, NRF_GPIO_PIN_NOPULL);
        if (nrf_gpio_pin_read(button_1_pin) != 0) {
            fd_boot_try_update();
        }
        fd_storage_fatfs_unmount();
    }

    static uint8_t buffer[4096];
    if (!fd_storage_fatfs_format(buffer, sizeof(buffer))) {
        // printf("error %d\n", error.code);
        return 1;
    }

    if (fd_storage_fatfs_mount()) {
        fd_boot_usb_start();
    }
    
    return 0;
}
