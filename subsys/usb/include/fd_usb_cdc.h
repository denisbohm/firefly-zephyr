#ifndef fd_usb_cdc_h
#define fd_usb_cdc_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fd_usb_cdc_rx_data_t)(const uint8_t *data, size_t length);

typedef struct {
    fd_usb_cdc_rx_data_t rx_data;
    const char *rx_event_name;
} fd_usb_cdc_configuration_t;

void fd_usb_cdc_initialize(fd_usb_cdc_configuration_t configuration);

bool fd_usb_cdc_tx_data(const uint8_t *data, size_t length);

#endif
