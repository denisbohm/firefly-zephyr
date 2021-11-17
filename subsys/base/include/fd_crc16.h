#ifndef fd_crc16_h
#define fd_crc16_h

#include <stddef.h>
#include <stdint.h>

uint16_t fd_crc16_update(const uint8_t *data, size_t length);

#endif
