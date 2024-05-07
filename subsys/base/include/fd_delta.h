#ifndef fd_delta_h
#define fd_delta_h

#include "fd_bit.h"
#include "fd_source.h"

fd_source_push()

typedef struct {
    uint32_t value_resolution;
    uint32_t delta_resolution;
} fd_delta_parameters_t;

uint32_t fd_delta_get_resolution(uint32_t value);

uint32_t fd_delta_zigzag_encode(int32_t value);
int32_t fd_delta_zigzag_decode(uint32_t value);

int32_t fd_delta_get_value(const void *objects, uint32_t index, size_t size, size_t offset);
void fd_delta_set_value(void *objects, uint32_t index, size_t size, size_t offset, int32_t value);

uint32_t fd_delta_get_delta_resolution(const void *objects, uint32_t count, size_t size, size_t offset);
bool fd_delta_encode(fd_bit_encoder_t *encoder, const void *objects, uint32_t count, size_t size, size_t offset, const fd_delta_parameters_t *parameters);
bool fd_delta_decode(fd_bit_decoder_t *decoder, void *objects, size_t size, size_t offset, uint32_t count, const fd_delta_parameters_t *parameters);

fd_source_pop()

#endif