#ifndef fd_crypto_sha_h
#define fd_crypto_sha_h

#include <stdbool.h>
#include <stdint.h>

#define fd_crypto_sha_context_size 128
#define fd_crypto_sha_block_size 64
#define fd_crypto_sha_hash_size 20

bool fd_crypto_sha_initialize(void *context);
bool fd_crypto_sha_update(void *context, const uint8_t *data, uint32_t length);
bool fd_crypto_sha_finalize(void *context, uint8_t *hash);

#endif
