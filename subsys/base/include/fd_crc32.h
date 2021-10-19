#ifndef fd_crc32_h
#define fd_crc32_h

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t crc;
    uint32_t count;
} fd_crc32_t;

void fd_crc32_initialize(fd_crc32_t *crc32);
void fd_crc32_update(fd_crc32_t *crc32, const uint8_t *data, size_t length);
void fd_crc32_finalize(fd_crc32_t *crc32);

#endif
