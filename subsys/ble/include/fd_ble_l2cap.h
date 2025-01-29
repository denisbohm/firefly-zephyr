#ifndef fd_ble_l2cap_h
#define fd_ble_l2cap_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct k_work_q *work_queue;
    void (*connected)(void);
    void (*disconnected)(void);
    void (*recv)(const uint8_t *data, size_t size);
    void (*sent)(void);
} fd_ble_l2cap_configuration_t;

void fd_ble_l2cap_initialize(fd_ble_l2cap_configuration_t configuration);

uint16_t fd_ble_l2cap_get_psm(void);

bool fd_ble_l2cap_send(const uint8_t *data, size_t length);

#endif
