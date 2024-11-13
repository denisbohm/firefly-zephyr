#include "fd_rpc_stream.h"

#include <stdio.h>
#include <string.h>

#include "fd_assert.h"
#include "fd_binary.h"

/*

Requirements:
- handle out of order packets
- handle duplicate packets
- retransmit if ack not received with timeout
- two way keep alive

Each stream packet has a header with 1-bit command/data flag.

Data
1) 1-bit command/data bit: zero
2) 31-bit sequence number
3) data content

Command
1) 1-bit command/data bit: one
2) 31-bit operation
3) command specific parameters

Operations
* Connect: two way negotiation to open connection - protocol version number, MTU, maximum flow window size (MFW)
* Disconect: close connection
* Keep Alive: sent periodically in both directions after an inactivity timeout
* ACK: periodically sent so sender can remove data from buffers
* NACK: sent when a gap is detected in the reveived data after some timeout
* ACKACK: sent to estimate RTT

Handshake
Client sends their desired values, server responds with modified values based on its capabilities

Connect Request 0x8000
8-bit protocol version numer
8-bit MFW
16-bit MTU

Connect Response 0x8001
8-bit result code (accepted = 0x00, rejected != 0x00)
8-bit protocol version numer
8-bit MFW
16-bit MTU

Disconnect 0x8002

Keep Alive 0x8004

ACK 0x8006
1-bit request ACKACK
31-bit sequence number

NACK 0x8008
1-bit zero
31-bit sequence number

ACKACK 0x800a
1-bit zero
31-bit sequence number

Client: 0x80000000 0x01 0x04 0x0400 // Connect Request, V1, MFW 4, MTU 1k
Server: 0x80000001 0x00 0x01 0x01 0x0100 // Connect Response, accepted, V1, MFW 1, MTU 256

Client: 0x00000000 ... // Data, sequence number 0
Server: 0x80000006 0x00000000 // ACK, sequence number 0
Client: 0x00000001 ... // Data, sequence number 1
Server: 0x80000006 0x00000001 // ACK, sequence number 1

Server: 0x00000000 ... // Data, sequence number 0
Client: 0x80000006 0x00000000 // ACK, sequnce number 0

Server: 0x80000002 // Disconnect

*/

fd_source_push()

void fd_rpc_stream_tracker_initialize(fd_rpc_stream_tracker_t *tracker, uint32_t timeout) {
    tracker->timeout = timeout;
    tracker->last_active_time = 0;
}

void fd_rpc_stream_tracker_reset(fd_rpc_stream_tracker_t *tracker) {
    tracker->last_active_time = 0;
}

void fd_rpc_stream_tracker_update(fd_rpc_stream_tracker_t *tracker, int64_t time) {
    tracker->last_active_time = time;
}

fd_rpc_stream_tracker_status_t fd_rpc_stream_tracker_check(fd_rpc_stream_tracker_t *tracker, int64_t time) {
    fd_assert(time >= tracker->last_active_time);
    int64_t delta = time - tracker->last_active_time;
    if (delta > tracker->timeout) {
        return fd_rpc_stream_tracker_status_timeout;
    }
    return fd_rpc_stream_tracker_status_ok;
}

void fd_rpc_stream_initialize(fd_rpc_stream_t *stream, const fd_rpc_stream_client_t *client) {
    memset(stream, 0, sizeof(*stream));
    stream->client = client;
    k_sem_init(&stream->send_semaphore, 1, 1);
    unsigned int count = k_sem_count_get(&stream->send_semaphore);
    fd_assert(count == 1);
    fd_rpc_stream_tracker_initialize(&stream->send.keep_alive, 2000);
    fd_rpc_stream_tracker_initialize(&stream->receive.keep_alive, 5000);
}

void fd_rpc_stream_disconnect(fd_rpc_stream_t *stream) {
    k_sem_reset(&stream->send_semaphore);
    k_sem_give(&stream->send_semaphore);
    unsigned int count = k_sem_count_get(&stream->send_semaphore);
    fd_assert(count == 1);
    stream->receive.sequence_number = 0;
    fd_rpc_stream_tracker_reset(&stream->receive.keep_alive);
    stream->send.sequence_number = 0;
    fd_rpc_stream_tracker_reset(&stream->send.keep_alive);
    stream->state = fd_rpc_stream_state_disconnected;
}

void fd_rpc_stream_send_command(fd_rpc_stream_t *stream, const uint8_t *data, size_t size) {
    fd_rpc_stream_tracker_update(&stream->send.keep_alive, k_uptime_get());
    stream->client->send_command(stream, data, size);
}

void fd_rpc_stream_send_connect_response(fd_rpc_stream_t *stream) {
    uint8_t data[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, fd_rpc_stream_command_connect_response | fd_rpc_stream_header_type_command);
    fd_binary_put_uint8(&binary, 0x00); // accepted
    fd_binary_put_uint8(&binary, fd_rpc_stream_version);
    fd_binary_put_uint8(&binary, 1); // maximum flow window size 1
    fd_binary_put_uint16(&binary, 1024); // MTU
    fd_assert(binary.errors == 0);
    fd_rpc_stream_send_command(stream, data, binary.put_index);
}

void fd_rpc_stream_send_disconnect(fd_rpc_stream_t *stream) {
    uint8_t data[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, fd_rpc_stream_command_disconnect | fd_rpc_stream_header_type_command);
    fd_assert(binary.errors == 0);
    fd_rpc_stream_send_command(stream, data, binary.put_index);

    fd_rpc_stream_disconnect(stream);
    stream->client->sent_disconnect(stream);
}

void fd_rpc_stream_send_keep_alive(fd_rpc_stream_t *stream) {
    uint8_t data[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, fd_rpc_stream_command_keep_alive | fd_rpc_stream_header_type_command);
    fd_assert(binary.errors == 0);
    fd_rpc_stream_send_command(stream, data, binary.put_index);
}

void fd_rpc_stream_send_ack(fd_rpc_stream_t *stream) {
    uint8_t data[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, fd_rpc_stream_command_ack | fd_rpc_stream_header_type_command);
    fd_binary_put_uint32(&binary, stream->receive.sequence_number);
    fd_assert(binary.errors == 0);
    fd_rpc_stream_send_command(stream, data, binary.put_index);
}

void fd_rpc_stream_send_nack(fd_rpc_stream_t *stream) {
    uint8_t data[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    fd_binary_put_uint32(&binary, fd_rpc_stream_command_nack | fd_rpc_stream_header_type_command);
    fd_binary_put_uint32(&binary, stream->send.sequence_number);
    fd_assert(binary.errors == 0);
    fd_rpc_stream_send_command(stream, data, binary.put_index);
}

bool fd_rpc_stream_wait_for_send_ack(fd_rpc_stream_t *stream) {
    int result = k_sem_take(&stream->send_semaphore, K_SECONDS(5));
    return result == 0;
}

bool fd_rpc_stream_send_data(fd_rpc_stream_t *stream, const uint8_t *data, size_t length) {
    fd_rpc_stream_tracker_update(&stream->send.keep_alive, k_uptime_get());

    uint8_t header[16];
    fd_binary_t binary;
    fd_binary_initialize(&binary, header, sizeof(header));
    fd_binary_put_uint32(&binary, stream->send.sequence_number);
    fd_assert(binary.errors == 0);
    return stream->client->send_data(stream, header, binary.put_index, data, length);
}

void fd_rpc_stream_receive_ack(fd_rpc_stream_t *stream, fd_binary_t *binary) {
    uint32_t serial_number = fd_binary_get_uint32(binary) & ~0x80000000;
    if (serial_number < stream->send.sequence_number) {
        // duplicate ack - ignore it
        return;
    }
    if (serial_number == stream->send.sequence_number) {
        ++stream->send.sequence_number;
        unsigned int count = k_sem_count_get(&stream->send_semaphore);
        fd_assert(count == 0);
        k_sem_give(&stream->send_semaphore);
        stream->client->received_ack(stream);
        return;
    }
    // should not happen
    fd_rpc_stream_disconnect(stream);
    fd_rpc_stream_send_disconnect(stream);
}

void fd_rpc_stream_receive_nack(fd_rpc_stream_t *stream, fd_binary_t *binary) {
    // MFW is 1, so a nack should not happen
    fd_rpc_stream_disconnect(stream);
    fd_rpc_stream_send_disconnect(stream);
}

void fd_rpc_stream_receive_connect_request(fd_rpc_stream_t *stream, fd_binary_t *binary) {
    fd_rpc_stream_disconnect(stream);

    int64_t now = k_uptime_get();
    fd_rpc_stream_tracker_update(&stream->receive.keep_alive, now);
    fd_rpc_stream_tracker_update(&stream->send.keep_alive, now);
    stream->state = fd_rpc_stream_state_connected;
    fd_rpc_stream_send_connect_response(stream);
    stream->client->received_connect(stream);
}

void fd_rpc_stream_receive_disconnect_request(fd_rpc_stream_t *stream) {
    fd_rpc_stream_disconnect(stream);
    stream->client->received_disconnect(stream);
}

void fd_rpc_stream_receive_data(fd_rpc_stream_t *stream, uint32_t sequence_number, const uint8_t *data, size_t length) {
    if (sequence_number < stream->receive.sequence_number) {
        // retransmitted data - ignore
        return;
    }
    if (sequence_number == stream->receive.sequence_number) {
        // next data in the sequence - process it
        fd_rpc_stream_send_ack(stream);
        stream->client->received_data(stream, data, length);
        ++stream->receive.sequence_number;
        return;
    }
    // data gap detected
    fd_rpc_stream_send_nack(stream);
}

void fd_rpc_stream_receive(fd_rpc_stream_t *stream, const uint8_t *data, size_t length) {
    fd_rpc_stream_tracker_update(&stream->receive.keep_alive, k_uptime_get());

    fd_assert(length >= 4);
    fd_binary_t binary;
    fd_binary_initialize(&binary, (uint8_t *)data, length);
    uint32_t header = fd_binary_get_uint32(&binary);
    if ((header & fd_rpc_stream_header_type_command) != 0) {
        uint32_t command = header & ~fd_rpc_stream_header_type_command;
        switch (command) {
            case fd_rpc_stream_command_connect_request:
                fd_rpc_stream_receive_connect_request(stream, &binary);
                break;
            case fd_rpc_stream_command_connect_response:
                fd_assert_fail("unexpected command");
                break;
            case fd_rpc_stream_command_disconnect:
                fd_rpc_stream_receive_disconnect_request(stream);
                break;
            case fd_rpc_stream_command_keep_alive:
                stream->client->received_keep_alive(stream);
                break;
            case fd_rpc_stream_command_ack:
                fd_rpc_stream_receive_ack(stream, &binary);
                break;
            case fd_rpc_stream_command_nack:
                fd_rpc_stream_receive_nack(stream, &binary);
                break;
            case fd_rpc_stream_command_ackack:
                fd_assert_fail("unimplemented command");
                break;
            default:
                fd_assert_fail("unknown command");
                break;
        }
    } else {
        if (stream->state == fd_rpc_stream_state_connected) {
            fd_rpc_stream_receive_data(stream, header, &data[4], length - 4);
        }
    }
}

void fd_rpc_stream_check(fd_rpc_stream_t *stream) {
    if (stream->state == fd_rpc_stream_state_disconnected) {
        return;
    }

    int64_t time = k_uptime_get();
    if (fd_rpc_stream_tracker_check(&stream->send.keep_alive, time) == fd_rpc_stream_tracker_status_timeout) {
        fd_rpc_stream_send_keep_alive(stream);
    }
    if (fd_rpc_stream_tracker_check(&stream->receive.keep_alive, time) == fd_rpc_stream_tracker_status_timeout) {
        fd_rpc_stream_send_disconnect(stream);
    }
}

#ifdef FD_RPC_STREAM_TEST

typedef struct {
    uint32_t keep_alive_count;
    uint32_t ack_count;
    fd_binary_t sent;
    fd_binary_t received_data;
} fd_rpc_stream_test_mock_t;

fd_rpc_stream_test_mock_t fd_rpc_stream_test_mock;

void fd_rpc_stream_mock_received_connect(struct fd_rpc_stream_s *stream) {
    fd_assert(stream->state == fd_rpc_stream_state_connected);
}

void fd_rpc_stream_mock_received_disconnect(struct fd_rpc_stream_s *stream) {
    fd_assert(stream->state == fd_rpc_stream_state_disconnected);
}

void fd_rpc_stream_mock_received_keep_alive(struct fd_rpc_stream_s *stream) {
    ++fd_rpc_stream_test_mock.keep_alive_count;
}

void fd_rpc_stream_mock_received_data(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length) {
    fd_binary_put_bytes(&fd_rpc_stream_test_mock.received_data, data, length);
}

void fd_rpc_stream_mock_received_ack(struct fd_rpc_stream_s *stream) {
    ++fd_rpc_stream_test_mock.ack_count;
}

void fd_rpc_stream_mock_sent_disconnect(struct fd_rpc_stream_s *stream) {
    fd_assert(stream->state == fd_rpc_stream_state_disconnected);
}

bool fd_rpc_stream_mock_send_command(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length) {
    fd_binary_put_bytes(&fd_rpc_stream_test_mock.sent, data, length);
    return true;
}

bool fd_rpc_stream_mock_send_data(struct fd_rpc_stream_s *stream, const uint8_t *header, size_t header_length, const uint8_t *data, size_t length) {
    fd_binary_put_bytes(&fd_rpc_stream_test_mock.sent, header, header_length);
    fd_binary_put_bytes(&fd_rpc_stream_test_mock.sent, data, length);
    return true;
}

typedef struct {
    uint32_t byte_count;
    uint32_t value;
} fd_rpc_stream_test_value_t;

void fd_rpc_stream_test_receive_values(fd_rpc_stream_t *stream, bool is_data, const fd_rpc_stream_test_value_t *values, size_t value_count) {
    uint8_t data[64];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    for (uint32_t i= 0; i < value_count; ++i) {
        const fd_rpc_stream_test_value_t *value = &values[i];
        switch (value->byte_count) {
            case 1:
                fd_binary_put_uint8(&binary, (uint8_t)value->value);
                break;
            case 2:
                fd_binary_put_uint16(&binary, (uint16_t)value->value);
                break;
            case 4:
                fd_binary_put_uint32(&binary, (uint32_t)value->value);
                break;
            default:
                break;
        }
    }

    fd_rpc_stream_receive(stream, data, binary.put_index);

    if (!is_data) {
        return;
    }
    fd_assert(fd_rpc_stream_test_mock.received_data.put_index == binary.put_index - 4);
    fd_assert(memcmp(fd_rpc_stream_test_mock.received_data.buffer, &data[4], binary.put_index - 4) == 0);
    fd_rpc_stream_test_mock.received_data.put_index = 0;
}

#define fd_rpc_stream_test_receive(name, ...) \
    fd_rpc_stream_test_value_t name[] =  {__VA_ARGS__};\
    fd_rpc_stream_test_receive_values(&stream, false, name, sizeof(name) / sizeof(name[0]))

#define fd_rpc_stream_test_receive_data(name, ...) \
    fd_rpc_stream_test_value_t name[] =  {__VA_ARGS__};\
    fd_rpc_stream_test_receive_values(&stream, true, name, sizeof(name) / sizeof(name[0]))

void fd_rpc_stream_test_sent_values(fd_rpc_stream_t *stream, const fd_rpc_stream_test_value_t *values, size_t value_count) {
    uint8_t data[64];
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, sizeof(data));
    for (uint32_t i= 0; i < value_count; ++i) {
        const fd_rpc_stream_test_value_t *value = &values[i];
        switch (value->byte_count) {
            case 1:
                fd_binary_put_uint8(&binary, (uint8_t)value->value);
                break;
            case 2:
                fd_binary_put_uint16(&binary, (uint16_t)value->value);
                break;
            case 4:
                fd_binary_put_uint32(&binary, (uint32_t)value->value);
                break;
            default:
                break;
        }
    }
    fd_assert(fd_rpc_stream_test_mock.sent.put_index == binary.put_index);
    fd_assert(memcmp(fd_rpc_stream_test_mock.sent.buffer, data, binary.put_index) == 0);
    fd_rpc_stream_test_mock.sent.put_index = 0;
}

#define fd_rpc_stream_test_sent(name, ...) \
    fd_rpc_stream_test_value_t name[] =  {__VA_ARGS__};\
    fd_rpc_stream_test_sent_values(&stream, name, sizeof(name) / sizeof(name[0]))

void fd_rpc_stream_test(void) {
    uint8_t sent_data[32];
    fd_binary_initialize(&fd_rpc_stream_test_mock.sent, sent_data, sizeof(sent_data));
    uint8_t received_data[32];
    fd_binary_initialize(&fd_rpc_stream_test_mock.received_data, received_data, sizeof(received_data));
    fd_rpc_stream_client_t client = {
        fd_rpc_stream_mock_received_connect,
        fd_rpc_stream_mock_received_disconnect,
        fd_rpc_stream_mock_received_keep_alive,
        fd_rpc_stream_mock_received_data,
        fd_rpc_stream_mock_received_ack,
        fd_rpc_stream_mock_sent_disconnect,
        fd_rpc_stream_mock_send_command,
        fd_rpc_stream_mock_send_data,
    };
    fd_rpc_stream_t stream;
    fd_rpc_stream_initialize(&stream, &client);

    // Client: Connect Request, V1, MFW 4, MTU 1k
    fd_rpc_stream_test_receive(c1, {4, 0x80000000}, {1, 0x01}, {1, 0x04}, {2, 0x0400});
    fd_assert(stream.state == fd_rpc_stream_state_connected);
    // Server: Connect Response, accepted, V1, MFW 1, MTU 1k
    fd_rpc_stream_test_sent(s1, {4, 0x80000001}, {1, 0x00}, {1, 0x01}, {1, 0x01}, {2, 0x0400});

    // Client: Data, sequence number 0, content 1, 2, 3
    fd_rpc_stream_test_receive_data(c2, {4, 0x00000000}, {1, 0x01}, {1, 0x02}, {1, 0x03});
    // Server: ACK, sequence number 0
    fd_rpc_stream_test_sent(s2, {4, 0x80000006}, {4, 0x00000000});
    // Client: Data, sequence number 1, content 4
    fd_rpc_stream_test_receive_data(c3, {4, 0x00000001}, {1, 0x04});
    // Server: ACK, sequence number 1
    fd_rpc_stream_test_sent(s3, {4, 0x80000006}, {4, 0x00000001});

    static const uint8_t sd1[] = {0x05, 0x06};
    fd_rpc_stream_send_data(&stream, sd1, sizeof(sd1));
    // Server: Data, sequence number 0, content 5, 6
    fd_rpc_stream_test_sent(s4, {4, 0x00000000}, {1, 0x05}, {1, 0x06});
    // Client: ACK, sequnce number 0
    fd_rpc_stream_test_receive(c4, {4, 0x80000006}, {4, 0x00000000});

    fd_assert(stream.state == fd_rpc_stream_state_connected);
    fd_rpc_stream_send_disconnect(&stream);
    // Server: 0x80000002 // Disconnect
    fd_rpc_stream_test_sent(s5, {4, 0x80000002});
    fd_assert(stream.state == fd_rpc_stream_state_disconnected);
}

int main(void) {
    fd_rpc_stream_test();
    return 0;
}

#endif

fd_source_pop()