#ifndef fd_packet_h
#define fd_packet_h

#include "fd_binary.h"

bool fd_packet_encode(fd_binary_t *binary);
bool fd_packet_decode(fd_binary_t *binary);

bool fd_packet_append(fd_binary_t *binary, uint8_t byte);

#endif
