#include "fd_boot.h"

#include <openssl/evp.h>

#include <stdio.h>
#include <string.h>

const uint32_t mock_code_base = 100;

#define mock_executable_size 32768
#define mock_update_size 32768

typedef struct {
    uint8_t executable_data[mock_executable_size];

    uint8_t update_data[mock_update_size];
    uint32_t update_length;

    uint8_t key[FD_BOOT_CRYPTO_KEY_SIZE];
} mock_t;

mock_t mock;

bool mock_get_executable_storage(fd_boot_info_executable_storage_t *storage, fd_boot_error_t *error) {
    *storage = (fd_boot_info_executable_storage_t) {
        .range = {
            .location = 0,
            .length = mock_executable_size,
        },
    };
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 0);
    return false;
#endif
}

bool mock_get_executable(fd_boot_info_executable_t *executable, fd_boot_error_t *error) {
    *executable = (fd_boot_info_executable_t) {
        .range = {
            .location = 0,
            .length = mock_executable_size,
        },
        .metadata_offset = 256,
    };
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 1);
    return false;
#endif
}

bool mock_get_update_storage(fd_boot_info_update_storage_t *storage, fd_boot_error_t *error) {
    *storage = (fd_boot_info_update_storage_t) {
        .range = {
            .location = 0,
            .length = mock.update_length,
        },
    };
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 2);
    return false;
#endif
}

bool mock_get_decryption(fd_boot_info_decryption_t *decryption, fd_boot_error_t *error) {
    memcpy(decryption->key.data, mock.key, FD_BOOT_CRYPTO_KEY_SIZE);
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 3);
    return false;
#endif
}

EVP_MD_CTX *mock_hash_mdctx;

bool mock_hash_initialize(fd_boot_hash_context_t *context, fd_boot_error_t *error) {
    mock_hash_mdctx = EVP_MD_CTX_create();
    if (mock_hash_mdctx == NULL) {
        fd_boot_set_error(error, mock_code_base + 4);
        return false;
    }
    if (EVP_DigestInit_ex(mock_hash_mdctx, EVP_sha1(), NULL) != 1) {
        fd_boot_set_error(error, mock_code_base + 4);
        return false;
    }
    return true;
}

bool mock_hash_update(fd_boot_hash_context_t *context, const uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    if (EVP_DigestUpdate(mock_hash_mdctx, data, length) != 1) {
        fd_boot_set_error(error, mock_code_base + 5);
        return false;
    }
    return true;
}

bool mock_hash_finalize(fd_boot_hash_context_t *context, fd_boot_hash_t *hash, fd_boot_error_t *error) {
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(mock_hash_mdctx, hash->data, &digest_len) != 1) {
        fd_boot_set_error(error, mock_code_base + 6);
        return false;
    }
    EVP_MD_CTX_destroy(mock_hash_mdctx);
    mock_hash_mdctx = NULL;
    return true;
}

EVP_CIPHER_CTX *decrypt_ctx;

bool mock_decrypt_initialize(
    fd_boot_decrypt_context_t *decrypt,
    const fd_boot_crypto_key_t *key,
    const fd_boot_crypto_initialization_vector_t *initialization_vector,
    fd_boot_error_t *error
) {
    decrypt_ctx = EVP_CIPHER_CTX_new();
    if (decrypt_ctx == NULL) {
        fd_boot_set_error(error, mock_code_base + 7);
        return false;
    }
    if (EVP_DecryptInit_ex(decrypt_ctx, EVP_aes_128_cbc(), NULL, key->data, initialization_vector->data) != 1) {
        fd_boot_set_error(error, mock_code_base + 7);
        return false;
    }
    EVP_CIPHER_CTX_set_padding(decrypt_ctx, 0);
    return true;
}

bool mock_decrypt_update(fd_boot_decrypt_context_t *decrypt, const uint8_t *in, uint8_t *out, uint32_t length, fd_boot_error_t *error) {
    int out_length;
    if (EVP_DecryptUpdate(decrypt_ctx, out, &out_length, in, length) != 1) {
        fd_boot_set_error(error, mock_code_base + 8);
        return false;
    }
    return true;
}

bool mock_decrypt_finalize(fd_boot_decrypt_context_t *decrypt, fd_boot_error_t *error) {
    uint8_t out[FD_BOOT_DECRYPT_BLOCK_SIZE];
    int out_length;
    if (EVP_DecryptFinal_ex(decrypt_ctx, out, &out_length) != 1) {
        fd_boot_set_error(error, mock_code_base + 9);
        return false;
    }
    EVP_CIPHER_CTX_free(decrypt_ctx);
    decrypt_ctx = NULL;
    return true;
}

bool mock_executable_read(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    memcpy(data, &mock.executable_data[location], length);
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 10);
    return false;
#endif
}

bool mock_executable_flasher_initialize(void *context, uint32_t location, uint32_t length, fd_boot_error_t *error) {
    memset(&mock.executable_data[location], 0xff, length);
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 11);
    return false;
#endif
}

bool mock_executable_flasher_write(void *context, uint32_t location, const uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    memcpy(&mock.executable_data[location], data, length);
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 12);
    return false;
#endif
}

bool mock_executable_flasher_finalize(void *context, fd_boot_error_t *error) {
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 13);
    return false;
#endif
}

bool mock_update_read(void *context, uint32_t location, uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    memcpy(data, &mock.update_data[location], length);
    return true;
#if 0
    fd_boot_set_error(error, mock_code_base + 13);
    return false;
#endif
}

void mock_feed(void) {
}

bool mock_can_upgrade(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version) {
    return true;
}

bool mock_can_downgrade(const fd_boot_version_t *executable_version, const fd_boot_version_t *update_version) {
    return false;
}

bool mock_can_install(const fd_boot_version_t *version) {
    return true;
}

void mock_progress(float amount) {
    printf("progress %0.2f\n", amount);
}

void update(fd_boot_update_interface_t *interface) {
    fd_boot_update_result_t result;
    fd_boot_error_t error;
    if (!fd_boot_update(interface, &result, &error)) {
        printf("error %d\n", error.code);
    } else {
        if (result.is_valid) {
            printf("is valid\n");
        } else {
            printf("issue %d\n", result.issue);
        }
    }
}

void read_update(const char *file_name) {
    FILE *update_bin = fopen(file_name, "rb");
    if (update_bin != 0) {
        fseek(update_bin, 0, SEEK_END);
        mock.update_length = ftell(update_bin);
        rewind(update_bin);
        fread(mock.update_data, 1, mock.update_length, update_bin);
    } else {
        printf("update not found: %s\n", file_name);
    }
}

int main(void) {
    memset(&mock, 0, sizeof(mock));

    fd_boot_update_interface_t interface = {
        .info = {
            .get_executable_storage = mock_get_executable_storage,
            .get_executable = mock_get_executable,
            .get_update_storage = mock_get_update_storage,
            .get_decryption = mock_get_decryption,
        },
        .progress = {
            .progress = mock_progress,
        },
        .watchdog = {
            .feed = mock_feed,
        },
        .hash = {
            .initialize = mock_hash_initialize,
            .update = mock_hash_update,
            .finalize = mock_hash_finalize,
        },
        .decrypt = {
            .initialize = mock_decrypt_initialize,
            .update = mock_decrypt_update,
            .finalize = mock_decrypt_finalize,
        },
        .executable_reader = {
            .context = 0,
            .read = mock_executable_read,
        },
        .executable_flasher = {
            .context = 0,
            .initialize = mock_executable_flasher_initialize,
            .write = mock_executable_flasher_write,
            .finalize = mock_executable_flasher_finalize,
        },
        .update_reader = {
            .context = 0,
            .read = mock_update_read,
        },
        .action = {
            .can_upgrade = mock_can_upgrade,
            .can_downgrade = mock_can_downgrade,
            .can_install = mock_can_install,
        }
    };

    memset(mock.executable_data, 0xff, sizeof(mock.executable_data));
    memset(mock.update_data, 0xff, sizeof(mock.update_data));
    uint8_t key[FD_BOOT_CRYPTO_KEY_SIZE] =
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    update(&interface);
    memcpy(mock.key, key, FD_BOOT_CRYPTO_KEY_SIZE);
    read_update("/Users/denis/sandbox/denisbohm/firefly-zephyr/tools/update/application_1_2_3.bin");
    update(&interface);
    update(&interface);
    mock.executable_data[0] = ~mock.executable_data[0];
    update(&interface);
    update(&interface);
    read_update("/Users/denis/sandbox/denisbohm/firefly-zephyr/tools/update/application_1_2_4.bin");
    update(&interface);
    update(&interface);
    read_update("/Users/denis/sandbox/denisbohm/firefly-zephyr/tools/update/application_1_2_3.bin");
    update(&interface);
    memset(mock.update_data, 0xff, sizeof(mock.update_data));
    update(&interface);
    return 0;
}