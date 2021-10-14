#include "fd_cobs.h"

size_t fd_cobs_decode(const uint8_t *input, size_t length, uint8_t *output) {
    const uint8_t *src = input;
    uint8_t *dst = output;
    for (uint8_t code = 0xff, block_length = 0; src < (input + length); --block_length) {
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
    return dst - output;
}

size_t fd_cobs_encode(const uint8_t *input, size_t length, uint8_t *output) {
    uint8_t *dst = output;
    uint8_t *code_pointer = dst++;
    uint8_t code = 1;
    for (const uint8_t *src = input; length-- != 0; ++src) {
        uint8_t byte = *src;
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
    return dst - output;
}

