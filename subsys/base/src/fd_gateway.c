#include "fd_gateway.h"

#include "fd_dispatch.h"

#include "fd_assert.h"

#include <string.h>

#ifndef fd_gateway_endpoint_limit
#define fd_gateway_endpoint_limit 5
#endif

typedef struct {
    uint32_t identifier;
    fd_gateway_endpoint_t endpoints[fd_gateway_endpoint_limit];
    uint32_t endpoint_count;
} fd_gateway_t;

fd_gateway_t fd_gateway;

fd_gateway_endpoint_t *fd_gateway_get_endpoint(fd_gateway_identifier_t identifier) {
    for (uint32_t i = 0; i < fd_gateway.endpoint_count; ++i) {
        fd_gateway_endpoint_t *endpoint = &fd_gateway.endpoints[i];
        if (identifier == endpoint->identifier) {
            return endpoint;
        }
    }
    return 0;
}

bool fd_gateway_is_endpoint(fd_gateway_identifier_t identifier) {
    return fd_gateway_get_endpoint(identifier) != 0;
}

void fd_gateway_add_endpoint(fd_gateway_endpoint_t *endpoint) {
    fd_assert(fd_gateway_is_endpoint(endpoint->identifier) == 0);
}

bool fd_gateway_transmit(fd_binary_t *message, fd_envelope_t *envelope) {
    if (!fd_envelope_encode(message, envelope)) {
        return false;
    }
    fd_gateway_endpoint_t *endpoint = fd_gateway_get_endpoint(envelope->target);
    if (endpoint == 0) {
        return false;
    }
    return endpoint->transmit(message->buffer, message->put_index);
}

bool fd_gateway_process(fd_binary_t *message) {
    fd_envelope_t envelope;
    if (!fd_envelope_decode(message, &envelope)) {
        return false;
    }

    if (envelope.target == fd_gateway.identifier) {
        return fd_dispatch_process(message, &envelope);
    }
    
    fd_gateway_endpoint_t *target = fd_gateway_get_endpoint(envelope.target);
    if (target == 0) {
        return false;
    }
    fd_envelope_encode(message, &envelope);
    return target->transmit(message->buffer, message->put_index);
}

void fd_gateway_initialize(fd_gateway_identifier_t identifer) {
    memset(&fd_gateway, 0, sizeof(fd_gateway));

    fd_gateway.identifier = identifer;
}
