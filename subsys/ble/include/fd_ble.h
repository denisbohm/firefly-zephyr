#ifndef fd_ble_h
#define fd_ble_h

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *custom_service_uuid;
} fd_ble_configuration_t;

void fd_ble_initialize(const fd_ble_configuration_t *configuration);

void fd_ble_start_advertising(void);

#endif