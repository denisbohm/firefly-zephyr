#include "fd_usb_cdc.h"

#include "fd_assert.h"

#include <device.h>
#include <drivers/uart.h>
#include <stdio.h>
#include <string.h>
#include <sys/ring_buffer.h>
#include <zephyr.h>

#include <usb/usb_device.h>

#define RING_BUF_SIZE 1024

typedef struct {
    fd_usb_cdc_configuration_t configuration;
    const struct device *dev;
    uint8_t rx_ring_buffer[RING_BUF_SIZE];
    struct ring_buf rx_ring_buf;
    uint32_t rx_count;
    uint32_t rx_drop_count;
} fd_usb_cdc_zephyr_t;

fd_usb_cdc_zephyr_t fd_usb_cdc_zephyr;

static void fd_usb_cdc_zephyr_irq_callback(const struct device *dev, void *user_data) {
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            uint8_t buffer[64];
            size_t len = MIN(ring_buf_space_get(&fd_usb_cdc_zephyr.rx_ring_buf), sizeof(buffer));
            int recv_len = uart_fifo_read(dev, buffer, len);
            if (recv_len > 0) {
                fd_usb_cdc_zephyr.rx_count += recv_len;
            }
            int rb_len = ring_buf_put(&fd_usb_cdc_zephyr.rx_ring_buf, buffer, recv_len);
            if (rb_len < recv_len) {
                fd_usb_cdc_zephyr.rx_drop_count += recv_len - rb_len;
            }
            fd_usb_cdc_zephyr.configuration.rx_ready();
        }

        if (uart_irq_tx_ready(dev)) {
            uart_irq_tx_disable(dev);
        }
    }
}

bool fd_usb_cdc_tx_data(const uint8_t *data, size_t length) {
    int send_len = uart_fifo_fill(fd_usb_cdc_zephyr.dev, data, length);
    return send_len == length;
}

size_t fd_usb_cdc_get_rx_data(uint8_t *buffer, size_t size) {
    return ring_buf_get(&fd_usb_cdc_zephyr.rx_ring_buf, buffer, size);
}

void fd_usb_cdc_initialize(fd_usb_cdc_configuration_t configuration) {
    memset(&fd_usb_cdc_zephyr, 0, sizeof(fd_usb_cdc_zephyr));

    fd_usb_cdc_zephyr.configuration = configuration;

    fd_usb_cdc_zephyr.dev = device_get_binding("CDC_ACM_0");
    if (!fd_usb_cdc_zephyr.dev) {
        return;
    }

#if 0
    int err = uart_line_ctrl_set(fd_usb_cdc_zephyr.dev, UART_LINE_CTRL_DCD, 1);
    fd_assert(err == 0);

    err = uart_line_ctrl_set(fd_usb_cdc_zephyr.dev, UART_LINE_CTRL_DSR, 1);
    fd_assert(err == 0);
#endif

    ring_buf_init(&fd_usb_cdc_zephyr.rx_ring_buf, sizeof(fd_usb_cdc_zephyr.rx_ring_buffer), fd_usb_cdc_zephyr.rx_ring_buffer);
    uart_irq_callback_set(fd_usb_cdc_zephyr.dev, fd_usb_cdc_zephyr_irq_callback);
    uart_irq_rx_enable(fd_usb_cdc_zephyr.dev);
}