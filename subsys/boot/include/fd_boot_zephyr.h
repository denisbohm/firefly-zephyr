#ifndef fd_boot_zephyr_h
#define fd_boot_zephyr_h

#include "fd_boot.h"

bool fd_boot_zephyr_update_storage_mount(fd_boot_error_t *error);
bool fd_boot_zephyr_update_storage_open(const char *file_name, fd_boot_error_t *error);
bool fd_boot_zephyr_update_storage_close(fd_boot_error_t *error);

bool fd_boot_zephyr_get_update_storage(
    fd_boot_info_update_storage_t *storage,
    fd_boot_error_t *error
);

bool fd_boot_zephyr_update_read(
    void *context,
    uint32_t location,
    uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
);

bool fd_boot_zephyr_hash_initialize(
    fd_boot_hash_context_t *context,
    fd_boot_error_t *error
);
bool fd_boot_zephyr_hash_update(
    fd_boot_hash_context_t *context,
    const uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
);
bool fd_boot_zephyr_hash_finalize(
    fd_boot_hash_context_t *context,
    fd_boot_hash_t *hash,
    fd_boot_error_t *error
);

bool fd_boot_zephyr_decrypt_initialize(
    fd_boot_decrypt_context_t *decrypt,
    const fd_boot_crypto_key_t *key,
    const fd_boot_crypto_initialization_vector_t *initialization_vector,
    fd_boot_error_t *error
);
bool fd_boot_zephyr_decrypt_update(
    fd_boot_decrypt_context_t *decrypt,
    const uint8_t *in,
    uint8_t *out,
    uint32_t length,
    fd_boot_error_t *error
);
bool fd_boot_zephyr_decrypt_finalize(
    fd_boot_decrypt_context_t *decrypt,
    fd_boot_error_t *error
);

#endif