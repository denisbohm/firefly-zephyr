#ifndef fd_crypto_aes_h
#define fd_crypto_aes_h

#include <stdbool.h>
#include <stdint.h>

#define fd_crypto_aes_context_size 128
#define fd_crypto_aes_key_size 16
#define fd_crypto_aes_iv_size 16

bool fd_crypto_aes_decrypt_initialize(void *context, const uint8_t *key, const uint8_t *iv);
bool fd_crypto_aes_decrypt_update(void *context, const uint8_t *in, uint8_t *out, uint32_t length);
bool fd_crypto_aes_decrypt_finalize(void *context);

#endif