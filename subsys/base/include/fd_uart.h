#ifndef fd_uart_h
#define fd_uart_h

#include "fd_gpio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    fd_uart_parity_none,
    fd_uart_parity_even,
    fd_uart_parity_odd,
} fd_uart_parity_t;

typedef enum {
    fd_uart_stop_bits_1,
    fd_uart_stop_bits_2,
} fd_uart_stop_bits_t;

typedef struct {
    const char *uart_device_name;
    fd_gpio_t tx_gpio;
    fd_gpio_t rx_gpio;
    uint32_t baud_rate;
    fd_uart_parity_t parity;
    fd_uart_stop_bits_t stop_bits;
    const char *tx_event_name;
    const char *rx_event_name;
} fd_uart_instance_t;

void fd_uart_initialize(void);

void fd_uart_instance_initialize(fd_uart_instance_t *instance);

size_t fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length);
void fd_uart_instance_tx_flush(fd_uart_instance_t *instance);

size_t fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length);

#endif
