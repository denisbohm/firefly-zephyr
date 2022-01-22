#include "fd_boot.h"
#include "fd_boot_zephyr.h"

#include <string.h>

#define FD_BOOT_EXECUTABLE_STORAGE_LOCATION 0
#define FD_BOOT_EXECUTABLE_STORAGE_LENGTH (128 * 1024)

#define FD_BOOT_EXECUTABLE_LOCATION 0
#define FD_BOOT_EXECUTABLE_METADATA_OFFSET 256

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

bool fd_boot_zephyr_get_update_storage(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error) {
    *storage = (fd_boot_info_update_storage_t) {
        .range = {
            .location = 0,
            .length = 0,
        },
    };
    fd_boot_set_error(error, 1);
    return false;
}

bool fd_boot_zephyr_get_decryption(fd_boot_info_decryption_t *decryption, fd_boot_error_t *error) {
    const uint8_t key[] = { FD_BOOT_DECRYPTION_KEY };
    memcpy(decryption->key.data, key, FD_BOOT_CRYPTO_KEY_SIZE);
    return true;
}

bool fd_boot_zephyr_executable_read(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    memcpy(data, (uint8_t *)location, length);
    return true;
}

bool fd_boot_zephyr_executable_flasher_initialize(void *context, uint32_t location, uint32_t length, fd_boot_error_t *error) {
    fd_boot_set_error(error, 1);
    return false;
}

bool fd_boot_zephyr_executable_flasher_write(void *context, uint32_t location, const uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    fd_boot_set_error(error, 1);
    return false;
}

bool fd_boot_zephyr_executable_flasher_finalize(void *context, fd_boot_error_t *error) {
    return true;
}

bool fd_boot_zephyr_update_read(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    fd_boot_set_error(error, 1);
    return false;
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

int main(void) {
    fd_boot_update_interface_t interface = {
        .info = {
            .get_executable_storage = fd_boot_zephyr_get_executable_storage,
            .get_executable = fd_boot_zephyr_get_executable,
            .get_update_storage = fd_boot_zephyr_get_update_storage,
            .get_decryption = fd_boot_zephyr_get_decryption,
        },
        .progress = {
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
            .initialize = fd_boot_zephyr_executable_flasher_initialize,
            .write = fd_boot_zephyr_executable_flasher_write,
            .finalize = fd_boot_zephyr_executable_flasher_finalize,
        },
        .update_reader = {
            .context = 0,
            .read = fd_boot_zephyr_update_read,
        },
        .action = {
            .can_upgrade = fd_boot_zephyr_can_upgrade,
            .can_downgrade = fd_boot_zephyr_can_downgrade,
            .can_install = fd_boot_zephyr_can_install,
        }
    };

    fd_boot_update_result_t result;
    fd_boot_error_t error;
    if (!fd_boot_update(&interface, &result, &error)) {
        // printf("error %d\n", error.code);
    } else {
        if (result.is_valid) {
            // printf("is valid\n");
        } else {
            // printf("issue %d\n", result.issue);
        }
    }

    return 1;
}
