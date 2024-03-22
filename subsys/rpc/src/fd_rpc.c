#include "fd_rpc.h"

#include <fd_assert.h>
#include <fd_cobs.h>
#include <fd_crc16.h>
#include <fd_unused.h>

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include <zephyr/net/buf.h>

fd_source_push()

typedef struct {
    uint32_t package_id;
    uint32_t service_id;
    uint32_t method_id;
    uint32_t invocation;
} fd_rpc_call_t;

typedef struct {
    bool allocated;
    fd_rpc_channel_t *channel;
    uint32_t invocation;
    const fd_rpc_method_t *method;
} fd_rpc_common_context_t;

typedef struct fd_rpc_server_context_s {
    fd_rpc_common_context_t common;
    const fd_rpc_method_server_t *server;
} fd_rpc_server_context_t;

typedef struct fd_rpc_client_context_s {
    fd_rpc_common_context_t common;
    const fd_rpc_method_client_t *client;
} fd_rpc_client_context_t;

typedef struct {
    const fd_rpc_method_t *method;
    const fd_rpc_method_server_t *server;
} fd_rpc_method_server_association_t;

#define fd_rpc_channel_packet_delimiter 0

typedef bool (*fd_rpc_channel_handler_t)(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);

bool fd_rpc_channel_server_initialize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);
bool fd_rpc_channel_server_request(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);
bool fd_rpc_channel_server_finalize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);
bool fd_rpc_channel_client_response(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);
bool fd_rpc_channel_client_finalize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size);

const fd_rpc_channel_handler_t fd_rpc_channel_handlers[] = {
    fd_rpc_channel_server_initialize,
    fd_rpc_channel_server_request,
    fd_rpc_channel_server_finalize,
    fd_rpc_channel_client_response,
    fd_rpc_channel_client_finalize,
};

NET_BUF_POOL_DEFINE(fd_rpc_pool, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_COUNT, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE, 0, NULL);

typedef struct {
    fd_rpc_method_server_association_t method_server_associations[CONFIG_FIREFLY_SUBSYS_RPC_SERVER_METHOD_LIMIT];
    size_t method_server_associations_count;

    fd_rpc_server_context_t server_contexts[CONFIG_FIREFLY_SUBSYS_RPC_SERVER_CONTEXT_LIMIT];
    fd_rpc_client_context_t client_contexts[CONFIG_FIREFLY_SUBSYS_RPC_CLIENT_CONTEXT_LIMIT];
} fd_rpc_t;

fd_rpc_t fd_rpc;

void fd_rpc_initialize(void) {
}

bool fd_rpc_channel_send_packet(fd_rpc_channel_t *channel, fd_rpc_channel_packet_type_t type, fd_binary_t *packet) {
    uint16_t crc16;
    uint16_t length;
    uint8_t type_data[5];
    pb_ostream_t stream = pb_ostream_from_buffer(type_data, sizeof(type_data));
    pb_encode_varint(&stream, type);
    const size_t header_size = sizeof(crc16) + sizeof(length);
    size_t prefix_size = header_size + stream.bytes_written;
    if ((prefix_size + packet->put_index) > packet->size) {
        return false;
    }
    uint8_t *data = packet->buffer;
    memmove(&data[prefix_size], data, packet->put_index);
    packet->put_index += prefix_size;
    memcpy(&data[header_size], type_data, stream.bytes_written);
    
    length = (uint16_t)(packet->put_index - header_size);
    size_t index = sizeof(crc16);
    data[index] = length & 0xff;
    data[index + 1] = (length >> 8) & 0xff;
    crc16 = fd_crc16_update(&data[sizeof(crc16)], packet->put_index - sizeof(crc16));
    index = 0;
    data[index] = crc16 & 0xff;
    data[index + 1] = (crc16 >> 8) & 0xff;
    uint8_t buffer[32];
    size_t encoded_length = fd_cobs_encode(data, packet->put_index, packet->size, buffer, sizeof(buffer));
    if (encoded_length == 0) {
        return false;
    }
    if (encoded_length >= packet->size) {
        return false;
    }
    data[encoded_length++] = fd_rpc_channel_packet_delimiter;

    return channel->packet_write(data, encoded_length);
}

bool fd_rpc_channel_received_packet(fd_rpc_channel_t *channel, fd_binary_t *packet) {
    uint8_t *data = packet->buffer;
    size_t size = packet->put_index;
    uint16_t crc16;
    uint16_t length;
    const size_t header_size = sizeof(crc16) + sizeof(length);
    if (size < header_size) {
        return false;
    }
    crc16 = (data[1] << 8) | data[0];
    length = (data[3] << 8) | data[2];
    size_t length_actual = size - header_size;
    uint16_t crc16_actual = fd_crc16_update(&data[sizeof(crc16)], size - sizeof(crc16));
    if (crc16_actual != crc16) {
        return false;
    }
    if (length_actual != length) {
        return false;
    }
    pb_istream_t stream = pb_istream_from_buffer(&data[header_size], size - header_size);
    uint32_t type;
    pb_decode_varint32(&stream, &type);
    if (type >= fd_rpc_channel_packet_type_count) {
        return true;
    }
    fd_rpc_channel_handler_t handler = fd_rpc_channel_handlers[type];
    if (handler == NULL) {
        return false;
    }
    size_t bytes_read = size - header_size - stream.bytes_left;
    if (!handler(channel, &data[header_size + bytes_read], stream.bytes_left)) {
        return false;
    }
    return true;
}

bool fd_rpc_channel_received_data(fd_rpc_channel_t *channel, fd_binary_t *packet, const uint8_t *data, size_t length) {
    bool result = true;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        if (byte != fd_rpc_channel_packet_delimiter) {
            fd_binary_put_uint8(packet, byte);
            continue;
        }
        if (packet->put_index == 0) {
            continue;
        }
        if (packet->errors != 0) {
            fd_binary_reset(packet);
            result = false;
            continue;
        }
        size_t size = fd_cobs_decode(packet->buffer, packet->put_index);
        fd_assert(size > 0);
        packet->put_index = size;
        bool result = fd_rpc_channel_received_packet(channel, packet);
        if (!result) {
            result = false;
        }
        fd_binary_reset(packet);
    }
    return result;
}

void fd_rpc_set_method_server_association(const fd_rpc_method_t *method, const fd_rpc_method_server_t *server) {
    fd_assert(fd_rpc.method_server_associations_count < ARRAY_SIZE(fd_rpc.method_server_associations));
    fd_rpc_method_server_association_t *association = &fd_rpc.method_server_associations[fd_rpc.method_server_associations_count++];
    association->method = method;
    association->server = server;
}

const fd_rpc_method_server_association_t *fd_rpc_get_method_server_association(fd_rpc_channel_t *channel, fd_rpc_call_t *call) {
    for (size_t i = 0; i < fd_rpc.method_server_associations_count; ++i) {
        const fd_rpc_method_server_association_t *association = &fd_rpc.method_server_associations[i];
        const fd_rpc_method_t *method = association->method;
        if (call->package_id != method->package_id) {
            continue;
        }
        if (call->service_id != method->service_id) {
            continue;
        }
        if (call->method_id != method->method_id) {
            continue;
        }
        return association;
    }
    return NULL;
}

void fd_rpc_server_free_space_increased(fd_rpc_channel_t *channel) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.server_contexts); ++i) {
        fd_rpc_server_context_t *context = &fd_rpc.server_contexts[i];
        if (!context->common.allocated) {
            continue;
        }
        if (context->common.channel != channel) {
            continue;
        }
        if (context->server->free_space_increased != NULL) {
            context->server->free_space_increased(context);
        }
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.client_contexts); ++i) {
        fd_rpc_client_context_t *context = &fd_rpc.client_contexts[i];
        if (!context->common.allocated) {
            continue;
        }
        if (context->common.channel != channel) {
            continue;
        }
        if (context->client->free_space_increased != NULL) {
            context->client->free_space_increased(context);
        }
    }
}

fd_rpc_server_context_t *fd_rpc_server_context_allocate(fd_rpc_channel_t *channel, uint32_t invocation, const fd_rpc_method_t *method, const fd_rpc_method_server_t *server) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.server_contexts); ++i) {
        fd_rpc_server_context_t *context = &fd_rpc.server_contexts[i];
        if (context->common.allocated) {
            continue;
        }
        context->common.allocated = true;
        context->common.channel = channel;
        context->common.invocation = invocation;
        context->common.method = method;
        context->server = server;
        channel->set_free_space_increased_callback(fd_rpc_server_free_space_increased);
        return context;
    }
    return NULL;
}

fd_rpc_server_context_t *fd_rpc_server_context_get(fd_rpc_channel_t *channel, const fd_rpc_call_t *call) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.server_contexts); ++i) {
        fd_rpc_server_context_t *context = &fd_rpc.server_contexts[i];
        if (!context->common.allocated) {
            continue;
        }
        if (context->common.channel != channel) {
            continue;
        }
        const fd_rpc_method_t *method = context->common.method;
        if (method->package_id != call->package_id) {
            continue;
        }
        if (method->service_id != call->service_id) {
            continue;
        }
        if (method->method_id != call->method_id) {
            continue;
        }
        if (context->common.invocation != call->invocation) {
            continue;
        }
        return context;
    }
    return NULL;
}

void fd_rpc_server_context_free(fd_rpc_server_context_t *context) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.server_contexts); ++i) {
        fd_rpc_server_context_t *a_context = &fd_rpc.server_contexts[i];
        if (a_context == context) {
            memset(context, 0, sizeof(*context));
            return;
        }
    }
}

fd_rpc_channel_t *fd_rpc_server_context_get_channel(fd_rpc_server_context_t *context) {
    if (context == NULL) {
        return NULL;
    }
    return context->common.channel;
}

fd_rpc_client_context_t *fd_rpc_client_context_allocate(fd_rpc_channel_t *channel, uint32_t invocation, const fd_rpc_method_t *method, const fd_rpc_method_client_t *client) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.client_contexts); ++i) {
        fd_rpc_client_context_t *context = &fd_rpc.client_contexts[i];
        if (context->common.allocated) {
            continue;
        }
        context->common.allocated = true;
        context->common.channel = channel;
        context->common.invocation = invocation;
        context->common.method = method;
        context->client = client;
        channel->set_free_space_increased_callback(fd_rpc_server_free_space_increased);
        return context;
    }
    return NULL;
}

fd_rpc_client_context_t *fd_rpc_client_context_get(fd_rpc_channel_t *channel, const fd_rpc_call_t *call) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.client_contexts); ++i) {
        fd_rpc_client_context_t *context = &fd_rpc.client_contexts[i];
        if (!context->common.allocated) {
            continue;
        }
        if (context->common.channel != channel) {
            continue;
        }
        const fd_rpc_method_t *method = context->common.method;
        if (method->package_id != call->package_id) {
            continue;
        }
        if (method->service_id != call->service_id) {
            continue;
        }
        if (method->method_id != call->method_id) {
            continue;
        }
        if (context->common.invocation != call->invocation) {
            continue;
        }
        return context;
    }
    return NULL;
}

void fd_rpc_client_context_free(fd_rpc_client_context_t *context) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.client_contexts); ++i) {
        fd_rpc_client_context_t *a_context = &fd_rpc.client_contexts[i];
        if (a_context == context) {
            memset(context, 0, sizeof(*context));
            return;
        }
    }
}

fd_rpc_channel_t *fd_rpc_client_context_get_channel(fd_rpc_client_context_t *context) {
    if (context == NULL) {
        return NULL;
    }
    return context->common.channel;
}

void fd_rpc_channel_opened(fd_rpc_channel_t *channel) {
}

void fd_rpc_channel_closed(fd_rpc_channel_t *channel) {
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.client_contexts); ++i) {
        fd_rpc_client_context_t *context = &fd_rpc.client_contexts[i];
        if (context->common.channel == channel) {
            fd_rpc_client_context_free(context);
        }
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(fd_rpc.server_contexts); ++i) {
        fd_rpc_server_context_t *context = &fd_rpc.server_contexts[i];
        if (context->common.channel == channel) {
            if (context->server->finalize != NULL) {
                bool result fd_unused = context->server->finalize(context);
            }
            fd_rpc_server_context_free(context);
        }
    }
}

bool fd_rpc_decode_call(pb_istream_t *stream, fd_rpc_call_t *call) {
    pb_decode_varint32(stream, &call->package_id);
    pb_decode_varint32(stream, &call->service_id);
    pb_decode_varint32(stream, &call->method_id);
    pb_decode_varint32(stream, &call->invocation);
    if (stream->errmsg != NULL) {
        return false;
    }
    return true;
}

bool fd_rpc_channel_server_initialize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size) {
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    fd_rpc_call_t call;
    if (!fd_rpc_decode_call(&stream, &call)) {
        return false;
    }
    
    const fd_rpc_method_server_association_t *association = fd_rpc_get_method_server_association(channel, &call);
    if (association == NULL) {
        return false;
    }

    fd_rpc_server_context_t *context = fd_rpc_server_context_allocate(channel, call.invocation, association->method, association->server);
    if (context == NULL) {
        return false;
    }

    if (context->server->initialize != NULL) {
        if (!context->server->initialize(context)) {
            fd_rpc_server_context_free(context);
            return false;
        }
    }

    return true;
}

bool fd_rpc_channel_server_request(fd_rpc_channel_t *channel, const uint8_t *data, size_t size) {
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    fd_rpc_call_t call;
    if (!fd_rpc_decode_call(&stream, &call)) {
        return false;
    }

    const fd_rpc_method_server_association_t *association = fd_rpc_get_method_server_association(channel, &call);
    if (association == NULL) {
        return false;
    }

    fd_rpc_server_context_t *context = NULL;
    if (association->method->server_streaming) {
        context = fd_rpc_server_context_get(channel, &call);
    } else {
        context = fd_rpc_server_context_allocate(channel, call.invocation, association->method, association->server);
    }
    if (context == NULL) {
        return false;
    }

    if (context->common.method->request_size > CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE) {
        return false;
    }

    struct net_buf *buf = net_buf_alloc(&fd_rpc_pool, K_MSEC(100));
    fd_assert(buf != NULL);
    if (buf == NULL) {
        return false;
    }
    void *request = net_buf_add(buf, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE);
    fd_assert(request != NULL);

    pb_decode(&stream, context->common.method->request_msgdesc, request);
    fd_assert(stream.errmsg == NULL);
    bool result = true;
    if (stream.errmsg == NULL) {
        if (context->server->request != NULL) {
            if (!context->server->request(context, request)) {
                result = false;
            }
        }
    } else {
        result = false;
    }

    net_buf_unref(buf);

    if (!association->method->server_streaming && !association->method->server_streaming) {
        fd_rpc_server_context_free(context);
    }

    return result;
}

bool fd_rpc_channel_server_finalize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size) {
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    fd_rpc_call_t call;
    if (!fd_rpc_decode_call(&stream, &call)) {
        return false;
    }

    fd_rpc_server_context_t *context = fd_rpc_server_context_get(channel, &call);
    if (context == NULL) {
        return false;
    }

    bool result = true;
    if (context->server->finalize != NULL) {
        result = context->server->finalize(context);
    }

    fd_rpc_server_context_free(context);

    return result;
}

bool fd_rpc_channel_client_response(fd_rpc_channel_t *channel, const uint8_t *data, size_t size) {
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    fd_rpc_call_t call;
    if (!fd_rpc_decode_call(&stream, &call)) {
        return false;
    }

    fd_rpc_client_context_t *context = fd_rpc_client_context_get(channel, &call);
    if (context == NULL) {
        return false;
    }

    if (context->common.method->response_size > CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE) {
        return false;
    }

    struct net_buf *buf = net_buf_alloc(&fd_rpc_pool, K_MSEC(100));
    fd_assert(buf != NULL);
    if (buf == NULL) {
        return false;
    }
    void *response = net_buf_add(buf, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE);
    fd_assert(response != NULL);

    bool result = true;
    pb_decode(&stream, context->common.method->response_msgdesc, response);
    if (stream.errmsg == NULL) {
        if (!context->client->response(context, response)) {
            result = false;
        }
    } else {
        result = false;
    }

    net_buf_unref(buf);

    return result;
}

bool fd_rpc_channel_client_finalize(fd_rpc_channel_t *channel, const uint8_t *data, size_t size) {
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    fd_rpc_call_t call;
    if (!fd_rpc_decode_call(&stream, &call)) {
        return false;
    }

    fd_rpc_client_context_t *context = fd_rpc_client_context_get(channel, &call);
    if (context == NULL) {
        return false;
    }

    bool result = context->client->finalize(context);
    fd_rpc_client_context_free(context);
    return result;
}

bool fd_rpc_send(fd_rpc_common_context_t *context, fd_rpc_channel_packet_type_t type, const pb_msgdesc_t *msgdesc, const void *message) {
    struct net_buf *buf = net_buf_alloc(&fd_rpc_pool, K_MSEC(100));
    fd_assert(buf != NULL);
    if (buf == NULL) {
        return false;
    }
    void *data = net_buf_add(buf, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE);
    fd_assert(data != NULL);

    pb_ostream_t stream = pb_ostream_from_buffer(data, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE);
    const fd_rpc_method_t *method = context->method;
    pb_encode_varint(&stream, method->package_id);
    pb_encode_varint(&stream, method->service_id);
    pb_encode_varint(&stream, method->method_id);
    pb_encode_varint(&stream, context->invocation);
    if (msgdesc != NULL) {
        pb_encode(&stream, msgdesc, message);
    }
    bool result = true;
    if (stream.errmsg == NULL) {
        fd_binary_t binary;
        fd_binary_initialize(&binary, data, CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE);
        binary.put_index = stream.bytes_written;
        if (!fd_rpc_channel_send_packet(context->channel, type, &binary)) {
            result = false;
        }
    } else {
        result = false;
    }

    net_buf_unref(buf);

    return result;
}

bool fd_rpc_server_send_client_response(fd_rpc_server_context_t *context, const void *response) {
    fd_assert(context != NULL);
    return fd_rpc_send(&context->common, fd_rpc_channel_packet_type_rpc_client_response, context->common.method->response_msgdesc, response);
}

bool fd_rpc_server_send_client_finalize(fd_rpc_server_context_t *context) {
    return fd_rpc_send(&context->common, fd_rpc_channel_packet_type_rpc_client_finalize, NULL, NULL);
}

bool fd_rpc_client_send_server_initialize(fd_rpc_client_context_t *context) {
    return fd_rpc_send(&context->common, fd_rpc_channel_packet_type_rpc_server_initialize, NULL, NULL);
}

bool fd_rpc_client_send_server_request(fd_rpc_client_context_t *context, const void *request) {
    fd_assert(context != NULL);
    return fd_rpc_send(&context->common, fd_rpc_channel_packet_type_rpc_server_request, context->common.method->request_msgdesc, request);
}

bool fd_rpc_client_send_server_finalize(fd_rpc_client_context_t *context) {
    return fd_rpc_send(&context->common, fd_rpc_channel_packet_type_rpc_server_finalize, NULL, NULL);
}

fd_source_pop()