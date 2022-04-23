#ifndef fd_ble_l2cap_h
#define fd_ble_l2cap_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void (*rx_ready)(void);
} fd_ble_l2cap_configuration_t;

void fd_ble_l2cap_initialize(fd_ble_l2cap_configuration_t configuration);

uint16_t fd_ble_l2cap_get_psm(void);

size_t fd_ble_l2cap_get_rx_data(uint8_t *buffer, size_t size);

bool fd_ble_l2cap_tx_data(const uint8_t *data, size_t length);

#endif
