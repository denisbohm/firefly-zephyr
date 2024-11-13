#ifndef fd_rpc_stream_h
#define fd_rpc_stream_h

#include "fd_source.h"

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

fd_source_push()

struct fd_rpc_stream_s;

typedef struct {
    void (*received_connect)(struct fd_rpc_stream_s *stream);
    void (*received_disconnect)(struct fd_rpc_stream_s *stream);
    void (*received_keep_alive)(struct fd_rpc_stream_s *stream);
    void (*received_data)(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length);
    void (*received_ack)(struct fd_rpc_stream_s *stream);
    void (*sent_disconnect)(struct fd_rpc_stream_s *stream);
    bool (*send_command)(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length);
    bool (*send_data)(struct fd_rpc_stream_s *stream, const uint8_t *header, size_t header_length, const uint8_t *data, size_t length);
} fd_rpc_stream_client_t;

#define fd_rpc_stream_version 1

#define fd_rpc_stream_header_type_command 0x80000000

typedef enum {
    fd_rpc_stream_command_connect_request = 0x00000000,
    fd_rpc_stream_command_connect_response = 0x00000001,
    fd_rpc_stream_command_disconnect = 0x00000002,
    fd_rpc_stream_command_keep_alive = 0x00000004,
    fd_rpc_stream_command_ack = 0x00000006,
    fd_rpc_stream_command_nack = 0x00000008,
    fd_rpc_stream_command_ackack = 0x0000000a,
} fd_rpc_stream_command_t;

typedef enum {
    fd_rpc_stream_state_disconnected,
    fd_rpc_stream_state_connected,
} fd_rpc_stream_state_t;

typedef struct {
    uint32_t timeout;
    int64_t last_active_time;
} fd_rpc_stream_tracker_t;

typedef enum {
    fd_rpc_stream_tracker_status_ok,
    fd_rpc_stream_tracker_status_timeout,
} fd_rpc_stream_tracker_status_t;

typedef struct {
    uint32_t sequence_number;
    fd_rpc_stream_tracker_t keep_alive;
} fd_rpc_stream_receive_t;

typedef struct {
    uint32_t sequence_number;
    fd_rpc_stream_tracker_t keep_alive;
} fd_rpc_stream_send_t;

typedef struct fd_rpc_stream_s {
    const fd_rpc_stream_client_t *client;
    fd_rpc_stream_state_t state;
    fd_rpc_stream_receive_t receive;
    fd_rpc_stream_send_t send;
    struct k_sem send_semaphore;
} fd_rpc_stream_t;

void fd_rpc_stream_initialize(fd_rpc_stream_t *stream, const fd_rpc_stream_client_t *client);

void fd_rpc_stream_check(fd_rpc_stream_t *stream);

void fd_rpc_stream_send_disconnect(fd_rpc_stream_t *stream);

void fd_rpc_stream_send_keep_alive(fd_rpc_stream_t *stream);

bool fd_rpc_stream_wait_for_send_ack(fd_rpc_stream_t *stream);
bool fd_rpc_stream_send_data(fd_rpc_stream_t *stream, const uint8_t *data, size_t length);

void fd_rpc_stream_receive(fd_rpc_stream_t *stream, const uint8_t *data, size_t length);

fd_source_pop()

#endif