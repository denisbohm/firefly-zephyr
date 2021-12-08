#include "fd_cobs.h"

#include "fd_assert.h"
#include "fd_fifo.h"

#include <string.h>

size_t fd_cobs_encode(uint8_t *data, size_t length, size_t size, uint8_t *buffer, size_t buffer_size) {
    fd_fifo_t fifo;
    fd_fifo_initialize(&fifo, buffer, buffer_size);
    size_t n = length;
    if (n > (buffer_size - 1)) {
        n = buffer_size - 1;
    }
    size_t offset = 0;
    for (offset = 0; offset < n; ++offset) {
        bool ok = fd_fifo_put(&fifo, data[offset]);
        fd_assert(ok);
    }

    uint8_t *dst = data;
    uint8_t *code_pointer = dst++;
    uint8_t code = 1;
    size_t remaining = length;
    while (remaining-- != 0) {
        uint8_t byte;
        bool ok = fd_fifo_get(&fifo, &byte);
        fd_assert(ok);
        if (offset < length) {
            ok = fd_fifo_put(&fifo, data[offset++]);
            fd_assert(ok);
        }
        if (byte != 0) {
            *dst++ = byte;
            ++code;
        }
        if ((byte == 0) || (code == 0xff)) {
            *code_pointer = code;
            code = 1;
            code_pointer = dst;
            if ((byte == 0) || (length != 0)) {
                ++dst;
            }
        }
    }
    *code_pointer = code;
    return dst - data;
}

size_t fd_cobs_decode(uint8_t *data, size_t length) {
    const uint8_t *src = data;
    uint8_t *dst = data;
    for (uint8_t code = 0xff, block_length = 0; src < (data + length); --block_length) {
        if (block_length) {
            *dst++ = *src++;
        } else {
            if (code != 0xff) {
                *dst++ = 0;
            }
            block_length = code = *src++;
            if (code == 0x00) {
                break;
            }
        }
    }
    return dst - data;
}
