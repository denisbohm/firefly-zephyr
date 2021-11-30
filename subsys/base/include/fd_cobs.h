#ifndef fd_cobs_h
#define fd_cobs_h

#include <stddef.h>
#include <stdint.h>

size_t fd_cobs_encode(uint8_t *data, size_t length, size_t size, uint8_t *buffer, size_t buffer_size);

size_t fd_cobs_decode(uint8_t *data, size_t length);

#endif
