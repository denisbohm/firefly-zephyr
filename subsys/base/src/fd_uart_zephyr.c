#include "fd_uart.h"

#include "fd_assert.h"
#include "fd_event.h"
#include "fd_fifo.h"

#include <zephyr.h>
#include <device.h>
#include <drivers/uart.h>

#include <stdio.h>
#include <string.h>

#ifndef fd_uart_info_limit
#define fd_uart_info_limit 4
#endif

typedef struct {
    uint8_t data[16];
} fd_uart_buffer_t;

typedef struct {
    int tx_done_count;
    int tx_aborted_count;
    int rx_rdy_count;
    int rx_buf_request_count;
    int rx_buf_released_count;
    int rx_disabled_count;
    int rx_stopped_count;
    int other_count;
} fd_uart_counts_t;

typedef struct {
    fd_uart_instance_t *instance;
    const struct device *device;
    uint32_t tx_event;
    uint32_t rx_event;
    fd_uart_buffer_t read_buffers[2];
    uint32_t read_buffer_index;
    fd_fifo_t rx_fifo;
    uint8_t rx_fifo_buffer[128];
    uint8_t tx_buffer[16];
    fd_fifo_t tx_fifo;
    bool is_tx_pending;
    uint8_t tx_fifo_buffer[128];
    bool is_rx_stopped;
    fd_uart_counts_t counts;
} fd_uart_info_t;

typedef struct {
    fd_uart_info_t infos[fd_uart_info_limit];
    uint32_t info_count;
} fd_uart_t;

fd_uart_t fd_uart;

void fd_uart_info_tx_flush(fd_uart_info_t *info) {
    while (info->is_tx_pending) {
    }
}

void fd_uart_info_rx_start(fd_uart_info_t *info) {
    fd_fifo_flush(&info->rx_fifo);
    info->is_rx_stopped = false;
    info->read_buffer_index = 0;
    fd_uart_buffer_t *buffer = &info->read_buffers[1];
    int result = uart_rx_enable(info->device, buffer->data, 16, 10);
    fd_assert(result == 0);
}

void fd_uart_info_tx_start(fd_uart_info_t *info) {
    if (info->is_rx_stopped) {
        fd_uart_info_rx_start(info);
    }

    fd_uart_info_tx_flush(info);

    uint8_t byte = 0;
    size_t i = 0;
    for (; i < 16; ++i) {
        if (!fd_fifo_get(&info->tx_fifo, &byte)) {
            break;
        }
        info->tx_buffer[i] = byte;
    }
    if (i > 0) {
        info->is_tx_pending = true;
        int result = uart_tx(info->device, info->tx_buffer, i, SYS_FOREVER_MS);
        fd_assert(result == 0);
    } else {
        fd_event_set_from_interrupt(info->tx_event);
    }
}

void fd_uart_callback_tx_done(fd_uart_info_t *info, struct uart_event *event) {
    info->is_tx_pending = false;
    fd_uart_info_tx_start(info);
}

void fd_uart_callback_rx_rdy(fd_uart_info_t *info, struct uart_event *event) {
    const uint8_t *data = event->data.rx.buf + event->data.rx.offset;
    size_t length = event->data.rx.len;
    for (size_t i = 0; i < length; ++i) {
        fd_fifo_put(&info->rx_fifo, data[i]);
    }
    fd_event_set_from_interrupt(info->rx_event);
}

void fd_uart_callback_buf_request(fd_uart_info_t *info, struct uart_event *event) {
    fd_uart_buffer_t *buffer = &info->read_buffers[info->read_buffer_index];
    if (++info->read_buffer_index >= 2) {
        info->read_buffer_index = 0;
    }
    int result = uart_rx_buf_rsp(info->device, buffer->data, 16);
    fd_assert(result == 0);
}

void fd_uart_callback_rx_stopped(fd_uart_info_t *uart) {
    uart->is_rx_stopped = true;
}

void fd_uart_callback(const struct device *device, struct uart_event *event, void *user_data) {
    fd_uart_info_t *info = (fd_uart_info_t *)user_data;
    switch (event->type) {
    case UART_TX_DONE:
        fd_uart_callback_tx_done(info, event);
        ++info->counts.tx_done_count;
        break;
    case UART_TX_ABORTED:
        ++info->counts.tx_aborted_count;
        break;
    case UART_RX_RDY:
        fd_uart_callback_rx_rdy(info, event);
        ++info->counts.rx_rdy_count;
        break;
    case UART_RX_BUF_REQUEST:
        fd_uart_callback_buf_request(info, event);
        ++info->counts.rx_buf_request_count;
        break;
    case UART_RX_BUF_RELEASED:
        ++info->counts.rx_buf_released_count;
        break;
    case UART_RX_DISABLED:
        ++info->counts.rx_disabled_count;
        break;
    case UART_RX_STOPPED:
        fd_uart_callback_rx_stopped(info);
        ++info->counts.rx_stopped_count;
        break;
    default:
        ++info->counts.other_count;
        break;
    }
}

fd_uart_info_t *fd_uart_get_info(fd_uart_instance_t *instance) {
    for (uint32_t i = 0; i < fd_uart_info_limit; ++i) {
        fd_uart_info_t *info = &fd_uart.infos[i];
        if (info->instance == instance) {
            return info;
        }
    }
    return 0;
}

size_t fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length) {
    fd_uart_info_t *info = fd_uart_get_info(instance);
    size_t count = 0;
    bool start = info->tx_fifo.head == info->tx_fifo.tail;
    for (size_t i = 0; i < length; ++i) {
        if (!fd_fifo_put(&info->tx_fifo, data[i])) {
            break;
        }
        ++count;
    }
    if (start) {
        fd_uart_info_tx_start(info);
    }
    return count;
}

void fd_uart_instance_tx_flush(fd_uart_instance_t *instance) {
    fd_uart_info_t *info = fd_uart_get_info(instance);
    fd_uart_info_tx_flush(info);
}

size_t fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length) {
    fd_uart_info_t *info = fd_uart_get_info(instance);
    size_t count = 0;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = 0;
        if (!fd_fifo_get(&info->rx_fifo, &byte)) {
            break;
        }
        data[i] = byte;
        ++count;
    }
    return count;
}

void fd_uart_instance_initialize(fd_uart_instance_t *instance) {
    fd_assert(fd_uart.info_count < fd_uart_info_limit);
    fd_uart_info_t *info = &fd_uart.infos[fd_uart.info_count++];

    info->instance = instance;

    fd_fifo_initialize(&info->tx_fifo, info->tx_fifo_buffer, sizeof(info->tx_fifo_buffer));
    fd_fifo_initialize(&info->rx_fifo, info->rx_fifo_buffer, sizeof(info->rx_fifo_buffer));

    info->tx_event = fd_event_get_identifier(instance->tx_event_name);
    info->rx_event = fd_event_get_identifier(instance->rx_event_name);

    info->device = device_get_binding(instance->uart_device_name);
    fd_assert(info->device != 0);
    int result = uart_callback_set(info->device, fd_uart_callback, info);
    fd_assert(result == 0);
    fd_uart_info_rx_start(info);
}

void fd_uart_initialize(void) {
    memset(&fd_uart, 0, sizeof(fd_uart));
}
