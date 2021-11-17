#include "fd_packet.h"

#include "fd_cobs.h"
#include "fd_crc16.h"

#define fd_packet_metadata_size 4

bool fd_packet_is_valid(const uint8_t *data, size_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, (uint8_t *)&data[length - fd_packet_metadata_size], fd_packet_metadata_size);
    size_t packet_length = fd_binary_get_uint16(&binary);
    if ((packet_length + fd_packet_metadata_size) != length) {
        return false;
    }
    uint16_t crc = fd_crc16_update(data, (uint32_t)(length - 2));
    uint16_t packet_crc = fd_binary_get_uint16(&binary);
    if (packet_crc != crc) {
        return false;
    }
    if (binary.errors) {
        return false;
    }
    return true;
}

void fd_packet_add_metadata(fd_binary_t *binary) {
    fd_binary_put_uint16(binary, (uint16_t)binary->put_index);
    uint16_t crc = fd_crc16_update(binary->buffer, binary->put_index);
    fd_binary_put_uint16(binary, crc);
}

bool fd_packet_encode(fd_binary_t *binary) {
    fd_packet_add_metadata(binary);
    uint8_t buffer[32];
    size_t length = fd_cobs_encode(binary->buffer, binary->put_index, buffer, sizeof(buffer));
    if (length > binary->size) {
        return false;
    }
    binary->put_index = length;
    return binary->errors == 0;
}

bool fd_packet_append(fd_binary_t *binary, uint8_t byte) {
    if (byte != 0) {
        fd_binary_put_uint8(binary, byte);
        return false;
    }

    if (binary->errors) {
        fd_binary_reset(binary);
        return false;
    }

    return true;
}

bool fd_packet_decode(fd_binary_t *binary) {
    if (binary->errors) {
        fd_binary_reset(binary);
        return false;
    }

    binary->put_index = fd_cobs_decode(binary->buffer, binary->put_index);
    if (binary->put_index < fd_packet_metadata_size) {
        return false;
    }
    if (!fd_packet_is_valid(binary->buffer, binary->put_index)) {
        fd_binary_reset(binary);
        return false;
    }

    binary->put_index -= fd_packet_metadata_size;
    return true;
}