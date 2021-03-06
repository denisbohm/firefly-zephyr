#include "fd_crypto_sha.h"

#include "sha.h"

bool fd_crypto_sha_initialize(void *context) {
    SHA_CTX *ctx = (SHA_CTX *)context;
    SHA1_Init(ctx);
    return true;
}

bool fd_crypto_sha_update(void *context, const uint8_t *data, uint32_t length) {
    SHA_CTX *ctx = (SHA_CTX *)context;
    SHA1_Update(ctx, (uint8_t *)data, length);
    return true;
}

bool fd_crypto_sha_finalize(void *context, uint8_t *hash) {
    SHA_CTX *ctx = (SHA_CTX *)context;
    SHA1_Final(hash, ctx);
    return true;
}
