#include "fd_crc16.h"

uint16_t fd_crc16_update(const uint8_t *data, size_t length) {
    uint32_t crc = 0;
    for (size_t j = 0; j < length; ++j) {
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
