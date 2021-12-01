#include "fd_crc16.h"

uint16_t fd_crc16_update(const uint8_t *data, size_t length) {
    uint32_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        uint32_t byte = data[i];
        crc ^= byte << 8;
        for (uint32_t j = 0; j < 8; ++j) {
            uint32_t temp = crc << 1;
            if (crc & 0x8000) {
                temp ^= 0x1021;
            }
            crc = temp;
        }
    }
    return crc;
}
