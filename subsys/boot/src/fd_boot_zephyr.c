#include "fd_boot_zephyr.h"

#include <mbedtls/platform.h>
#include <mbedtls/cipher.h>
#include <mbedtls/md.h>

#include <fs/fs.h>
#include <ff.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>

#include <string.h>

typedef struct {
    FIL file;
    uint32_t length;
} fd_boot_zephyr_update_storage_t;

fd_boot_zephyr_update_storage_t fd_boot_zephyr_update_storage;

bool fd_boot_zephyr_update_storage_open(const char *file_name, fd_boot_error_t *error) {
    FILINFO info;
    FRESULT result = f_stat(file_name, &info);
    if (result != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    fd_boot_zephyr_update_storage.length = info.fsize;
    result = f_open(&fd_boot_zephyr_update_storage.file, file_name, FA_READ);
    if (result != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_zephyr_update_storage_close(fd_boot_error_t *error) {
    FRESULT result = f_close(&fd_boot_zephyr_update_storage.file);
    if (result != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_zephyr_get_update_storage(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error) {
    *storage = (fd_boot_info_update_storage_t) {
        .range = {
            .location = 0,
            .length = fd_boot_zephyr_update_storage.length,
        },
    };
    return true;
}

bool fd_boot_zephyr_update_read(
    void *context,
    uint32_t location,
    uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
) {
    uint32_t actual = 0;
    FRESULT result = f_lseek(&fd_boot_zephyr_update_storage.file, location);
    if (result != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    result = f_read(&fd_boot_zephyr_update_storage.file, data, length, &actual);
    if (result != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}
 
typedef struct {
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t ctx;
} fd_boot_zephyr_hash_t;

fd_boot_zephyr_hash_t fd_boot_zephyr_hash;

bool fd_boot_zephyr_hash_initialize(
    fd_boot_hash_context_t *context,
    fd_boot_error_t *error
) {
    fd_boot_zephyr_hash.md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (fd_boot_zephyr_hash.md_info == NULL) {
        fd_boot_set_error(error, 1);
        return false;
    }
    mbedtls_md_init(&fd_boot_zephyr_hash.ctx);
    if (mbedtls_md_init_ctx(&fd_boot_zephyr_hash.ctx, fd_boot_zephyr_hash.md_info) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    mbedtls_md_starts(&fd_boot_zephyr_hash.ctx);
    return true;
}
 
bool fd_boot_zephyr_hash_update(
    fd_boot_hash_context_t *context,
    const uint8_t *data,
    uint32_t length,
    fd_boot_error_t *error
) {
    mbedtls_md_update(&fd_boot_zephyr_hash.ctx, data, length);
    return true;
}

bool fd_boot_zephyr_hash_finalize(
    fd_boot_hash_context_t *context,
    fd_boot_hash_t *hash,
    fd_boot_error_t *error
) {
    mbedtls_md_finish(&fd_boot_zephyr_hash.ctx, hash->data);
    mbedtls_md_free(&fd_boot_zephyr_hash.ctx);
    return true;
}

typedef struct {
    mbedtls_cipher_context_t ctx;
} fd_boot_zephyr_decrypt_t;

fd_boot_zephyr_decrypt_t fd_boot_zephyr_decrypt;

bool fd_boot_zephyr_decrypt_initialize(
    fd_boot_decrypt_context_t *decrypt,
    const fd_boot_crypto_key_t *key,
    const fd_boot_crypto_initialization_vector_t *initialization_vector,
    fd_boot_error_t *error
) {
    mbedtls_cipher_init(&fd_boot_zephyr_decrypt.ctx);
    if (mbedtls_cipher_setup(&fd_boot_zephyr_decrypt.ctx, mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CBC)) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    if (mbedtls_cipher_set_padding_mode(&fd_boot_zephyr_decrypt.ctx, MBEDTLS_PADDING_NONE) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    if (mbedtls_cipher_setkey(&fd_boot_zephyr_decrypt.ctx, key->data, sizeof(key->data) * 8, MBEDTLS_DECRYPT) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    if (mbedtls_cipher_set_iv(&fd_boot_zephyr_decrypt.ctx, initialization_vector->data, sizeof(initialization_vector->data)) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_zephyr_decrypt_update(
    fd_boot_decrypt_context_t *decrypt,
    const uint8_t *in,
    uint8_t *out,
    uint32_t length,
    fd_boot_error_t *error
) {
    size_t out_length;
    if (mbedtls_cipher_update(&fd_boot_zephyr_decrypt.ctx, in, length, out, &out_length) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_zephyr_decrypt_finalize(
    fd_boot_decrypt_context_t *decrypt,
    fd_boot_error_t *error
) {
    uint8_t out[FD_BOOT_DECRYPT_BLOCK_SIZE];
    size_t length = 0;
    if (mbedtls_cipher_finish(&fd_boot_zephyr_decrypt.ctx, out, &length) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_zephyr_executable_read(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    memcpy(data, (uint8_t *)location, length);
    return true;
}

bool fd_boot_zephyr_executor_cleanup(fd_boot_error_t *error) {
    irq_lock();
    return true;
}
