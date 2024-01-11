#include "fd_rpc_channel_usb.h"

#include "fd_assert.h"
#include "fd_binary.h"
#include "fd_rpc.h"
#include "fd_unused.h"
#include "fd_usb.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    fd_rpc_channel_usb_configuration_t configuration;
    fd_rpc_channel_usb_consumer_t consumer;

    uint8_t serial_number[16];

    fd_rpc_channel_t channel;
    fd_rpc_channel_free_space_increased_callback_t free_space_increased_callback;
    struct k_timer timer;
    struct k_work connected_work;
    struct k_work disconnected_work;
    struct k_work rx_work;
    struct k_work callback_work;

    const struct device *device;
    bool powered;
    bool connected;
    uint8_t rx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_USB_RX_BUFFER_SIZE];
    struct ring_buf rx_ringbuf;
    uint8_t tx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_USB_TX_BUFFER_SIZE];
    struct ring_buf tx_ringbuf;
    uint8_t rx_packet_buffer[CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE];
    fd_binary_t rx_packet;

    uint32_t tx_ring_count;
    size_t tx_ring_bytes;
    uint32_t tx_uart_count;
    size_t tx_uart_bytes;
} fd_rpc_channel_usb_t;

fd_rpc_channel_usb_t fd_rpc_channel_usb;

const size_t fd_rpc_channel_usb_low_water_mark = sizeof(fd_rpc_channel_usb.tx_ring_buffer) / 2;

void fd_rpc_channel_usb_rx_work(struct k_work *context fd_unused) {
    uint8_t data[64];
    while (true) {
        size_t length = ring_buf_get(&fd_rpc_channel_usb.rx_ringbuf, data, sizeof(data));
        if (length <= 0) {
            break;
        }
        fd_rpc_channel_received_data(&fd_rpc_channel_usb.channel, &fd_rpc_channel_usb.rx_packet, data, length);
    }
}

void fd_rpc_channel_usb_rx_irq(void) {
    uint8_t buffer[64];
    size_t length = MIN(ring_buf_space_get(&fd_rpc_channel_usb.rx_ringbuf), sizeof(buffer));
    int recv_length = uart_fifo_read(fd_rpc_channel_usb.device, buffer, length);
    fd_assert(recv_length >= 0);
    if (recv_length < 0) {
        // Failed to read UART FIFO
        recv_length = 0;
    };
    int rb_length = ring_buf_put(&fd_rpc_channel_usb.rx_ringbuf, buffer, recv_length);
    fd_assert(rb_length == recv_length);
    if (rb_length < recv_length) {
        // Dropped recv_len - rb_len bytes
    }
    k_work_submit_to_queue(fd_rpc_channel_usb.configuration.work_queue, &fd_rpc_channel_usb.rx_work);
}

void fd_rpc_channel_usb_callback_work(struct k_work *context fd_unused) {
    if (fd_rpc_channel_usb.free_space_increased_callback != NULL) {
        fd_rpc_channel_usb.free_space_increased_callback(&fd_rpc_channel_usb.channel);
    }
}

void fd_rpc_channel_usb_tx_irq(void) {
    uint8_t *buffer = NULL;
    uint32_t length = ring_buf_get_claim(&fd_rpc_channel_usb.tx_ringbuf, &buffer, sizeof(fd_rpc_channel_usb.tx_ring_buffer));
    if (length == 0) {
        int result = ring_buf_get_finish(&fd_rpc_channel_usb.tx_ringbuf, length);
        fd_assert(result == 0);
        uart_irq_tx_disable(fd_rpc_channel_usb.device);
    } else {
        int send_length = uart_fifo_fill(fd_rpc_channel_usb.device, buffer, length);
        fd_assert(send_length <= (int)length);
        int result = ring_buf_get_finish(&fd_rpc_channel_usb.tx_ringbuf, send_length);
        fd_assert(result == 0);
        ++fd_rpc_channel_usb.tx_uart_count;
        fd_rpc_channel_usb.tx_uart_bytes += send_length;
    }
    k_work_submit_to_queue(fd_rpc_channel_usb.configuration.work_queue, &fd_rpc_channel_usb.callback_work);
}

void fd_rpc_channel_usb_cdc_uart_irq_handler(const struct device *dev, void *user_data fd_unused) {
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
            fd_rpc_channel_usb_rx_irq();
		}
        if (uart_irq_tx_ready(dev)) {
            fd_rpc_channel_usb_tx_irq();
        }
	}
}

void fd_rpc_channel_usb_cdc_connected_work(struct k_work *work fd_unused) {
	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    int result = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
    fd_assert(result == 0);
    result = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
    fd_assert(result == 0);

    fd_rpc_channel_opened(&fd_rpc_channel_usb.channel);

    if (fd_rpc_channel_usb.consumer.connected != NULL) {
        fd_rpc_channel_usb.consumer.connected();
    }
}

void fd_rpc_channel_usb_cdc_disconnected_work(struct k_work *work fd_unused) {
    fd_rpc_channel_closed(&fd_rpc_channel_usb.channel);

    if (fd_rpc_channel_usb.consumer.disconnected != NULL) {
        fd_rpc_channel_usb.consumer.disconnected();
    }

	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    int result = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 0);
    fd_assert(result == 0);
    result = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 0);
    fd_assert(result == 0);
}

void fd_rpc_channel_usb_cdc_timer(struct k_timer *timer fd_unused) {
	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

    uint32_t dtr = 0;
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    bool connected = dtr != 0;
    if (connected && !fd_rpc_channel_usb.connected) {
        fd_rpc_channel_usb.connected = true;
        k_work_submit_to_queue(fd_rpc_channel_usb.configuration.work_queue, &fd_rpc_channel_usb.connected_work);
    }
    if (!connected && fd_rpc_channel_usb.connected) {
        fd_rpc_channel_usb.connected = false;
        k_work_submit_to_queue(fd_rpc_channel_usb.configuration.work_queue, &fd_rpc_channel_usb.disconnected_work);
    }

    if (fd_rpc_channel_usb.connected) {
        uart_irq_tx_enable(fd_rpc_channel_usb.device);
    }
}

bool fd_rpc_channel_usb_packet_write(const uint8_t *data, size_t size) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_usb.tx_ringbuf);
    if (size > space) {
        uart_irq_tx_enable(fd_rpc_channel_usb.device);
        return false;
    }
    uint32_t length = ring_buf_put(&fd_rpc_channel_usb.tx_ringbuf, data, size);
    fd_assert(length == size);
    ++fd_rpc_channel_usb.tx_ring_count;
    fd_rpc_channel_usb.tx_ring_bytes += length;
//    uart_irq_tx_enable(fd_rpc_channel_usb.device);
    return true;
}

size_t fd_rpc_channel_usb_get_free_space(void) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_usb.tx_ringbuf);
    return space;
}

void fd_rpc_channel_usb_set_free_space_increased_callback(fd_rpc_channel_free_space_increased_callback_t callback) {
    fd_rpc_channel_usb.free_space_increased_callback = callback;
}

void fd_rpc_channel_usb_set_serial_number(const char *serial_number) {
    // unfortunately, the USB serial number is accessed before usb_enable is called, so need to set it up here -denis
    snprintf(fd_rpc_channel_usb.serial_number, sizeof(fd_rpc_channel_usb.serial_number), "%s", serial_number);
}

// This has weak linkage in Zephyr, so we override it here to provide our serial number.
uint8_t *usb_update_sn_string_descriptor(void) {
	return fd_rpc_channel_usb.serial_number;
}

void fd_rpc_channel_usb_set_consumer(const fd_rpc_channel_usb_consumer_t *consumer) {
    fd_rpc_channel_usb.consumer = *consumer;
}

void fd_rpc_channel_usb_initialize(const fd_rpc_channel_usb_configuration_t *configuration) {
    fd_rpc_channel_usb.configuration = *configuration;

    fd_rpc_channel_usb.channel = (fd_rpc_channel_t) { 
        .packet_write = fd_rpc_channel_usb_packet_write,
        .get_free_space = fd_rpc_channel_usb_get_free_space,
        .set_free_space_increased_callback = fd_rpc_channel_usb_set_free_space_increased_callback,
    };

    fd_binary_initialize(&fd_rpc_channel_usb.rx_packet, fd_rpc_channel_usb.rx_packet_buffer, sizeof(fd_rpc_channel_usb.rx_packet_buffer));
    
    k_work_init(&fd_rpc_channel_usb.rx_work, fd_rpc_channel_usb_rx_work);
    k_work_init(&fd_rpc_channel_usb.callback_work, fd_rpc_channel_usb_callback_work);

	ring_buf_init(&fd_rpc_channel_usb.rx_ringbuf, sizeof(fd_rpc_channel_usb.rx_ring_buffer), fd_rpc_channel_usb.rx_ring_buffer);
	ring_buf_init(&fd_rpc_channel_usb.tx_ringbuf, sizeof(fd_rpc_channel_usb.tx_ring_buffer), fd_rpc_channel_usb.tx_ring_buffer);

    k_work_init(&fd_rpc_channel_usb.connected_work, fd_rpc_channel_usb_cdc_connected_work);
    k_work_init(&fd_rpc_channel_usb.disconnected_work, fd_rpc_channel_usb_cdc_disconnected_work);
    k_timer_init(&fd_rpc_channel_usb.timer, fd_rpc_channel_usb_cdc_timer, 0);

	fd_rpc_channel_usb.device = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	fd_assert(device_is_ready(fd_rpc_channel_usb.device));
    static const fd_usb_configuration_t usb_configuration = {
        .connected = NULL,
        .disconnected = NULL,
    };
    fd_usb_initialize(&usb_configuration);

    uart_irq_callback_set(fd_rpc_channel_usb.device, fd_rpc_channel_usb_cdc_uart_irq_handler);
	uart_irq_rx_enable(fd_rpc_channel_usb.device);

    k_timer_start(&fd_rpc_channel_usb.timer, K_MSEC(250), K_MSEC(250));
}