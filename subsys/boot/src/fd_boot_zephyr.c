#include "fd_boot_zephyr.h"

#include <mbedtls/platform.h>
#include <mbedtls/cipher.h>
#include <mbedtls/md.h>
 
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
    mbedtls_cipher_setup(&fd_boot_zephyr_decrypt.ctx, mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CBC));
    if (mbedtls_cipher_setkey(&fd_boot_zephyr_decrypt.ctx, key->data, sizeof(key->data), MBEDTLS_DECRYPT) != 0) {
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
    if (mbedtls_cipher_finish(&fd_boot_zephyr_decrypt.ctx, NULL, NULL) != 0) {
        fd_boot_set_error(error, 1);
        return false;
    }
}