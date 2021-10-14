#ifndef fd_packet_h
#define fd_packet_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool fd_packet_is_valid(const uint8_t *data, size_t length);

void fd_packet_add_metadata(uint8_t *data, size_t length);

#endif
