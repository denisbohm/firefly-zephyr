#ifndef fd_ble_h
#define fd_ble_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *custom_service_uuid;
    void (*connected)(void *connection);
    void (*disconnected)(void *connection);
    void (*mtu_updated)(uint16_t mtu);
} fd_ble_configuration_t;

void fd_ble_initialize(const fd_ble_configuration_t *configuration);

void fd_ble_start_advertising(void);

bool fd_ble_is_connected(void);

void *fd_ble_get_connection(void);

#endif