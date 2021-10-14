#include "fd_packet.h"

static
uint16_t crc16_update(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0;
    for (uint32_t j = 0; j < length; ++j) {
        uint32_t byte = data[j];
        crc ^= byte << 8;
        for (uint32_t i = 0; i < 8; ++i) {
            uint32_t temp = crc << 1;
            if (crc & 0x8000) {
                temp ^= 0x1021;
            }
            crc = temp;
        }
    }
    return crc;
}

bool fd_packet_is_valid(const uint8_t *data, size_t length) {
    if (length < 3) {
        return false;
    }
    uint32_t packet_length = data[length - 3];
    if (packet_length != length) {
        return false;
    }
    uint16_t crc = crc16_update(data, (uint32_t)(length - 2));
    uint16_t packet_crc = data[length - 2] | (data[length - 1] << 8);
    return packet_crc == crc;
}

void fd_packet_add_metadata(uint8_t *data, size_t length) {
    data[length - 3] = length;
    uint16_t crc = crc16_update(data, (uint32_t)(length - 2));
    data[length - 2] = crc & 0xff;
    data[length - 1] = (crc >> 8) & 0xff;
}

