#ifndef fd_gateway_h
#define fd_gateway_h

#include "fd_envelope.h"
#include "fd_binary.h"

#include <stddef.h>

typedef uint8_t fd_gateway_identifier_t;

typedef bool (*fd_gateway_transmit_t)(const uint8_t *data, size_t size);

typedef struct {
    fd_gateway_identifier_t identifier;
    fd_gateway_transmit_t transmit;
} fd_gateway_endpoint_t;

void fd_gateway_initialize(fd_gateway_identifier_t identifier);

void fd_gateway_add_endpoint(fd_gateway_endpoint_t *endpoint);

bool fd_gateway_process(fd_binary_t *message);

bool fd_gateway_transmit(fd_binary_t *message, fd_envelope_t *envelope);

#endif