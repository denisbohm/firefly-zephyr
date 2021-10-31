#ifndef fd_uart_h
#define fd_uart_h

#include "fd_error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *uart_device_name;
    const char *tx_event_name;
    const char *rx_event_name;
} fd_uart_instance_t;

void fd_uart_initialize(void);

void fd_uart_instance_initialize(fd_uart_instance_t *instance);

bool fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length, fd_error_t *error);
void fd_uart_instance_tx_flush(fd_uart_instance_t *instance);

bool fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length, fd_error_t *error);

#endif
