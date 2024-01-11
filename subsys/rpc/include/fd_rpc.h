#ifndef fd_rpc_h
#define fd_rpc_h

#include "fd_binary.h"

#include <pb.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fd_rpc_channel_packet_type_rpc_server_initialize,
    fd_rpc_channel_packet_type_rpc_server_request,
    fd_rpc_channel_packet_type_rpc_server_finalize,
    fd_rpc_channel_packet_type_rpc_client_response,
    fd_rpc_channel_packet_type_rpc_client_finalize,

    fd_rpc_channel_packet_type_count
} fd_rpc_channel_packet_type_t;

typedef struct fd_rpc_channel_s fd_rpc_channel_t;

typedef void (*fd_rpc_channel_free_space_increased_callback_t)(fd_rpc_channel_t *channel);

typedef struct fd_rpc_channel_s {
    bool (*packet_write)(const uint8_t *data, size_t size);

    size_t (*get_free_space)(void);
    void (*set_free_space_increased_callback)(fd_rpc_channel_free_space_increased_callback_t callback);
} fd_rpc_channel_t;

typedef struct {
    uint32_t package_id;
    uint32_t service_id;
    uint32_t method_id;
    uint32_t request_size;
    const pb_msgdesc_t *request_msgdesc;
    uint32_t response_size;
    const pb_msgdesc_t *response_msgdesc;
    bool server_streaming;
    bool client_streaming;
    uint32_t access;
} fd_rpc_method_t;

typedef struct fd_rpc_client_context_s fd_rpc_client_context_t;

typedef struct {
    bool (*response)(fd_rpc_client_context_t *context, const void *response);
    bool (*finalize)(fd_rpc_client_context_t *context);

    void (*free_space_increased)(fd_rpc_client_context_t *context);
} fd_rpc_method_client_t;

typedef struct fd_rpc_server_context_s fd_rpc_server_context_t;

typedef struct {
    bool (*initialize)(fd_rpc_server_context_t *context);
    bool (*request)(fd_rpc_server_context_t *context, const void *request);
    bool (*finalize)(fd_rpc_server_context_t *context);

    void (*free_space_increased)(fd_rpc_server_context_t *context);
} fd_rpc_method_server_t;

void fd_rpc_initialize(void);

void fd_rpc_set_method_server_association(const fd_rpc_method_t *method, const fd_rpc_method_server_t *server);

void fd_rpc_channel_opened(fd_rpc_channel_t *channel);
void fd_rpc_channel_closed(fd_rpc_channel_t *channel);

fd_rpc_server_context_t *fd_rpc_server_context_allocate(fd_rpc_channel_t *channel, uint32_t invocation, const fd_rpc_method_t *method, const fd_rpc_method_server_t *server);
void fd_rpc_server_context_free(fd_rpc_server_context_t *context);
fd_rpc_channel_t *fd_rpc_server_context_get_channel(fd_rpc_server_context_t *context);

fd_rpc_client_context_t *fd_rpc_client_context_allocate(fd_rpc_channel_t *channel, uint32_t invocation, const fd_rpc_method_t *method, const fd_rpc_method_client_t *client);
void fd_rpc_client_context_free(fd_rpc_client_context_t *context);
fd_rpc_channel_t *fd_rpc_client_context_get_channel(fd_rpc_client_context_t *context);

bool fd_rpc_server_send_client_response(fd_rpc_server_context_t *context, const void *response);
bool fd_rpc_server_send_client_finalize(fd_rpc_server_context_t *context);

bool fd_rpc_client_send_server_initialize(fd_rpc_client_context_t *context);
bool fd_rpc_client_send_server_request(fd_rpc_client_context_t *context, const void *request);
bool fd_rpc_client_send_server_finalize(fd_rpc_client_context_t *context);

bool fd_rpc_channel_send_packet(fd_rpc_channel_t *channel, fd_rpc_channel_packet_type_t type, fd_binary_t *packet);
bool fd_rpc_channel_received_data(fd_rpc_channel_t *channel, fd_binary_t *packet, const uint8_t *data, size_t length);

#endif