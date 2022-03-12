#ifndef fd_usb_cdc_h
#define fd_usb_cdc_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void (*rx_ready)(void);
} fd_usb_cdc_configuration_t;

void fd_usb_cdc_initialize(fd_usb_cdc_configuration_t configuration);

size_t fd_usb_cdc_get_rx_data(uint8_t *buffer, size_t size);

bool fd_usb_cdc_tx_data(const uint8_t *data, size_t length);

#endif
