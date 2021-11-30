#include "fd_gateway.h"

#include "fd_dispatch.h"

#include "fd_assert.h"

#include <string.h>

#ifndef fd_gateway_item_limit
#define fd_gateway_item_limit 5
#endif

typedef struct {
    fd_gateway_target_t target;
    fd_gateway_endpoint_t endpoint;
} fd_gateway_item_t;

typedef struct {
    uint32_t target;
    fd_gateway_item_t items[fd_gateway_item_limit];
    uint32_t item_count;
} fd_gateway_t;

fd_gateway_t fd_gateway;

fd_gateway_item_t *fd_gateway_get_item(fd_gateway_target_t target) {
    for (uint32_t i = 0; i < fd_gateway.item_count; ++i) {
        fd_gateway_item_t *item = &fd_gateway.items[i];
        if (target == item->target) {
            return item;
        }
    }
    return 0;
}

void fd_gateway_add_endpoint(fd_gateway_target_t target, fd_gateway_endpoint_t endpoint) {
    fd_assert(fd_gateway_get_item(target) == 0);
    fd_gateway_item_t *item = &fd_gateway.items[fd_gateway.item_count++];
    item->target = target;
    item->endpoint = endpoint;
}

bool fd_gateway_transmit(fd_binary_t *message, fd_envelope_t *envelope) {
    if (!fd_envelope_encode(message, envelope)) {
        return false;
    }
    fd_gateway_item_t *item = fd_gateway_get_item(envelope->target);
    if (item == 0) {
        return false;
    }
    return item->endpoint.transmit(message->buffer, message->put_index);
}

bool fd_gateway_process(fd_binary_t *message) {
    fd_envelope_t envelope;
    if (!fd_envelope_decode(message, &envelope)) {
        return false;
    }

    if (envelope.target == fd_gateway.target) {
        return fd_dispatch_process(message, &envelope);
    }
    
    fd_gateway_item_t *item = fd_gateway_get_item(envelope.target);
    if (item == 0) {
        return false;
    }
    fd_envelope_encode(message, &envelope);
    return item->endpoint.transmit(message->buffer, message->put_index);
}

void fd_gateway_initialize(fd_gateway_target_t target) {
    memset(&fd_gateway, 0, sizeof(fd_gateway));

    fd_gateway.target = target;
}
