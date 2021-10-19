#ifndef fd_cobs_h
#define fd_cobs_h

#include <stddef.h>
#include <stdint.h>

size_t fd_cobs_decode(const uint8_t *input, size_t length, uint8_t *output);

size_t fd_cobs_encode(const uint8_t *input, size_t length, uint8_t *output);

#endif
