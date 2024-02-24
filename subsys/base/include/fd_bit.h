#ifndef fd_bit_h
#define fd_bit_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t bit_count;
    uint32_t bit_index;
} fd_bit_decoder_t;

void fd_bit_decoder_initialize(fd_bit_decoder_t *input, const uint8_t *data, size_t size, uint32_t bit_count);
uint32_t fd_bit_decoder_read(fd_bit_decoder_t *input, uint32_t bit_count);

typedef struct {
    uint8_t *data;
    size_t size;
    uint32_t bit_count;
    uint32_t index;
    uint32_t byte;
    bool overflow;
} fd_bit_encoder_t;

void fd_bit_encoder_initialize(fd_bit_encoder_t *output, uint8_t *data, size_t size);
void fd_bit_encoder_write(fd_bit_encoder_t *output, uint32_t value, uint32_t bit_count);
void fd_bit_encoder_finalize(fd_bit_encoder_t *output);

#endif