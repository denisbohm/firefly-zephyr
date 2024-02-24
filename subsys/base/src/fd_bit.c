#include "fd_bit.h"

#include "fd_assert.h"

void fd_bit_decoder_initialize(fd_bit_decoder_t *input, const uint8_t *data, size_t size, uint32_t bit_count) {
	input->data = data;
	input->size = size;
	input->bit_count = bit_count;
	input->bit_index = 0;
}

uint32_t fd_bit_decoder_read(fd_bit_decoder_t *input, uint32_t bit_count) {
    fd_assert((input->bit_index + bit_count) <= input->bit_count);
    uint32_t extracted_bit_count = 0;
    uint32_t mask = 0xffffffff >> (32 - bit_count);
    uint32_t value = 0;
    while (extracted_bit_count < bit_count) {
        uint32_t index = input->bit_index / 8;
        fd_assert(index < input->size);
        uint8_t byte = input->data[index];
        uint32_t byte_bit_index = input->bit_index % 8;
        uint32_t byte_bit_count = 8 - byte_bit_index;
        uint32_t remaining_bit_count = bit_count - extracted_bit_count;
        if (byte_bit_count > remaining_bit_count) {
            byte_bit_count = remaining_bit_count;
        }
        value |= (mask & (byte >> byte_bit_index)) << extracted_bit_count;
        mask = mask >> byte_bit_count;
        extracted_bit_count += byte_bit_count;
        input->bit_index += byte_bit_count;
    }
    return value;
}

void fd_bit_encoder_initialize(fd_bit_encoder_t *output, uint8_t *data, size_t size) {
	output->data = data;
	output->size = size;
	output->bit_count = 0;
	output->index = 0;
	output->byte = 0;
	output->overflow = false;
}

void fd_bit_encoder_write(fd_bit_encoder_t *output, uint32_t value, uint32_t bit_count) {
	fd_assert((value >> bit_count) == 0);
    if ((output->bit_count + bit_count) > (output->size * 8)) {
    	output->overflow = true;
        return;
    }
    uint32_t bit_index = output->bit_count % 8;
    output->byte |= value << bit_index;
    bit_index += bit_count;
    while (bit_index >= 8) {
    	output->data[output->index++] = (uint8_t)output->byte;
    	output->byte >>= 8;
        bit_index -= 8;
    }
    fd_assert(output->byte < 256);
    output->bit_count += bit_count;
}

void fd_bit_encoder_finalize(fd_bit_encoder_t *output) {
    fd_assert(!output->overflow);
    uint32_t bit_index = output->bit_count % 8;
    if (bit_index > 0) {
    	output->data[output->index++] = output->byte;
    }
    fd_assert(output->index == (output->bit_count + 7) / 8);
}
