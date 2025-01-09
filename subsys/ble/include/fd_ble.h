#ifndef fd_ble_h
#define fd_ble_h

#include "fd_source.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

fd_source_push()

typedef struct {
    const uint8_t *custom_service_uuid;
    const uint8_t *manufacturer_data;
    size_t manufacturer_data_size;
    void (*connected)(void *connection);
    void (*disconnected)(void *connection);
    void (*mtu_updated)(uint16_t mtu);
} fd_ble_configuration_t;

bool fd_ble_initialize(const fd_ble_configuration_t *configuration);

size_t fd_ble_get_version(uint8_t *git_hash, size_t size);

void fd_ble_set_service_uuid(const uint8_t uuid[16]);
void fd_ble_set_manufacturer_data(const uint8_t *data, size_t size);

void fd_ble_start_advertising_with_id(uint32_t id);
void fd_ble_start_advertising(void);
void fd_ble_stop_advertising(void);

bool fd_ble_is_connected(void);
void *fd_ble_get_connection(void);
uint8_t fd_ble_get_disconnect_reason(void);
void fd_ble_disconnect(void);

int8_t fd_ble_get_advertising_tx_power(uint32_t id);
void fd_ble_set_advertising_tx_power(uint32_t id, int8_t tx_power);
int8_t fd_ble_get_connection_tx_power(void *connection);
void fd_ble_set_connection_tx_power(void *connection, int8_t tx_power);

fd_source_pop()

#endif