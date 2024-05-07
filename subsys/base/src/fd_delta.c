#include "fd_delta.h"

#include "fd_assert.h"
#include "fd_bit.h"

fd_source_push()

uint32_t fd_delta_get_resolution(uint32_t value) {
    if (value == 0) {
        return 0;
    }
    return 32 - __builtin_clz(value);
}

uint32_t fd_delta_zigzag_encode(int32_t value) {
    return (2 * value) ^ (value >> (sizeof(int32_t) * 8 - 1));
}

int32_t fd_delta_zigzag_decode(uint32_t value) {
    return (value >> 1) ^ (-(value & 1));
}

int32_t fd_delta_get_value(const void *objects, uint32_t index, size_t size, size_t offset) {
    int32_t *field = (int32_t *)((uint8_t *)objects + size * index + offset);
    return *field;
}

void fd_delta_set_value(void *objects, uint32_t index, size_t size, size_t offset, int32_t value) {
    int32_t *field = (int32_t *)((uint8_t *)objects + size * index + offset);
    *field = value;
}

uint32_t fd_delta_get_delta_resolution(const void *objects, uint32_t count, size_t size, size_t offset) {
    uint32_t resolution = 0;
    int32_t previous_value = fd_delta_get_value(objects, 0, size, offset);
    for (uint32_t i = 1; i < count; ++i) {
        int32_t value = fd_delta_get_value(objects, i, size, offset);
        int32_t delta = value - previous_value;
        uint32_t encoded_delta = fd_delta_zigzag_encode(delta);
        uint32_t encoded_delta_resolution = fd_delta_get_resolution(encoded_delta);
        if (encoded_delta_resolution > resolution) {
            resolution = encoded_delta_resolution;
        }
        previous_value = value;
    }
    return resolution;
}

bool fd_delta_encode(fd_bit_encoder_t *encoder, const void *objects, uint32_t count, size_t size, size_t offset, const fd_delta_parameters_t *parameters) {
    int32_t previous_value = fd_delta_get_value(objects, 0, size, offset);
    fd_bit_encoder_write(encoder, fd_delta_zigzag_encode(previous_value), parameters->value_resolution);
    if (parameters->delta_resolution == 0) {
        return !encoder->overflow;
    }
    for (uint32_t i = 1; i < count; ++i) {
        int32_t value = fd_delta_get_value(objects, i, size, offset);
        int32_t delta = value - previous_value;
        uint32_t encoded_delta = fd_delta_zigzag_encode(delta);
        fd_assert(fd_delta_get_resolution(encoded_delta) <= parameters->delta_resolution);
        fd_bit_encoder_write(encoder, encoded_delta, parameters->delta_resolution);
        previous_value = value;
    }
    return !encoder->overflow;
}

bool fd_delta_decode(fd_bit_decoder_t *decoder, void *objects, uint32_t count, size_t size, size_t offset, const fd_delta_parameters_t *parameters) {
    int32_t previous_value = fd_delta_zigzag_decode(fd_bit_decoder_read(decoder, parameters->value_resolution));
    fd_delta_set_value(objects, 0, size, offset, previous_value);
    if (parameters->delta_resolution == 0) {
        for (uint32_t i = 1; i < count; ++i) {
            fd_delta_set_value(objects, i, size, offset, previous_value);
        }
        return true;
    }
    for (uint32_t i = 1; i < count; ++i) {
        uint32_t encoded_delta = fd_bit_decoder_read(decoder, parameters->delta_resolution);
        int32_t delta = fd_delta_zigzag_decode(encoded_delta);
        int32_t value = previous_value + delta;
        fd_delta_set_value(objects, i, size, offset, value);
        previous_value = value;
    }
    return true;
}

#if 0
void fd_delta_test_zigzag(void) {
    // 10 = -2 zz 11
    // 11 = -1 zz 01
    // 00 = 0 zz 00
    // 01 = 1 zz 10
    int value_resolution = 2;
    for (int32_t value = -2; value < 2; ++value) {
        uint8_t data[2];
        fd_bit_encoder_t bit_encoder;
        fd_bit_encoder_initialize(&bit_encoder, data, sizeof(data));
        uint32_t zigzag = fd_delta_zigzag_encode(value);
        fd_assert(fd_delta_get_resolution(zigzag) <= value_resolution);
        fd_bit_encoder_write(&bit_encoder, zigzag, value_resolution);
        fd_bit_encoder_finalize(&bit_encoder);
        fd_assert(bit_encoder.bit_count <= value_resolution);
        fd_bit_decoder_t bit_decoder;
        fd_bit_decoder_initialize(&bit_decoder, data, sizeof(data), bit_encoder.bit_count);
        int32_t result = fd_delta_zigzag_decode(fd_bit_decoder_read(&bit_decoder, value_resolution));
        fd_assert(result == value);
    }
}
#endif

fd_source_pop()
