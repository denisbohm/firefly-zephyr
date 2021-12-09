#include "fd_uart.h"

#include "fd_assert.h"
#include "fd_fifo.h"

#include <zephyr.h>

#include <nrf.h>

#include <string.h>

#ifndef FD_UART_DEVICE_LIMIT
#define FD_UART_DEVICE_LIMIT 4
#endif

#ifndef FD_UART_DEVICE_RX_FIFO_LIMIT
#define FD_UART_DEVICE_RX_FIFO_LIMIT 300
#endif

#ifndef FD_UART_DEVICE_TX_FIFO_LIMIT
#define FD_UART_DEVICE_TX_FIFO_LIMIT 300
#endif

#ifndef FD_UART_DEVICE_TX_DMA_LIMIT
#define FD_UART_DEVICE_TX_DMA_LIMIT 300
#endif

typedef struct {
    uint8_t dma_data[2];
    uint32_t dma_index;
    uint8_t fifo_data[FD_UART_DEVICE_RX_FIFO_LIMIT];
    fd_fifo_t fifo;
} fd_uart_device_rx_t;

typedef struct {
    uint8_t dma_data[FD_UART_DEVICE_TX_DMA_LIMIT];
    uint8_t fifo_data[FD_UART_DEVICE_TX_FIFO_LIMIT];
    fd_fifo_t fifo;
} fd_uart_device_tx_t;

typedef struct {
    fd_uart_instance_t *instance;
    NRF_UARTE_Type *uarte;
    fd_uart_device_rx_t rx;
    fd_uart_device_tx_t tx;
} fd_uart_device_t;

typedef struct {
    fd_uart_device_t devices[FD_UART_DEVICE_LIMIT];
} fd_uart_t;

fd_uart_t fd_uart;

void fd_uart_rx_set_next_dma_buffer(fd_uart_device_t *device) {
    NRF_UARTE_Type *uarte = device->uarte;
    fd_uart_device_rx_t *rx = &device->rx;
    uarte->RXD.PTR = rx->dma_data[rx->dma_index];
    rx->dma_index = (rx->dma_index + 1) & 0x1;
}

void fd_uart_rx_start(fd_uart_device_t *device) {
    fd_uart_rx_set_next_dma_buffer(device);
    NRF_UARTE_Type *uarte = device->uarte;
    uarte->TASKS_STARTRX = 1;
}

void fd_uart_tx_next_dma_buffer(fd_uart_device_t *device) {
    fd_uart_device_tx_t *tx = &device->tx;
    fd_fifo_t *fifo = &tx->fifo;
    uint8_t byte = 0;
    size_t i;
    for (i = 0; i < sizeof(tx->dma_data); ++i) {
        if (fd_fifo_get(fifo, &byte)) {
            tx->dma_data[i] = byte;
        }
    }
    if (i > 0) {
        NRF_UARTE_Type *uarte = device->uarte;
        uarte->TXD.MAXCNT = i;
        uarte->TASKS_STARTTX = 1;
    }
}

int fd_uart_serial_handler(fd_uart_device_t *device) {
    NRF_UARTE_Type *uarte = device->uarte;

    fd_uart_device_rx_t *rx = &device->rx;
    if (uarte->EVENTS_RXSTARTED) {
        fd_uart_rx_set_next_dma_buffer(device);
    }
    if (uarte->EVENTS_RXDRDY) {
        // generated when a byte is received, but RXD.AMOUNT is not updated ye
        // and the byte may not have been written to RAM by DMA yet
    }
    if (uarte->EVENTS_ENDRX) {
        fd_assert(uarte->RXD.AMOUNT == 1);
        // get receivied byte
        uint8_t byte = rx->dma_data[rx->dma_index];
        fd_uart_rx_start(device);
        fd_fifo_put(&rx->fifo, byte);
    }
    if (uarte->EVENTS_RXTO) {
        // generated when rx stop task is complete
    }

    if (uarte->EVENTS_TXSTARTED) {
        // generated when transmitter has started but no bytes sent yet
    }
    if (uarte->EVENTS_TXDRDY) {
        // generated after each byte is transmitted
    }
    if (uarte->EVENTS_ENDTX) {
        fd_uart_tx_next_dma_buffer(device);
    }
    if (uarte->EVENTS_TXSTOPPED) {
        // generated when tx stop task is complete
    }

    return 0;
}

ISR_DIRECT_DECLARE(fd_uart_serial0_handler) {
    return fd_uart_serial_handler(&fd_uart.devices[0]);
}

ISR_DIRECT_DECLARE(fd_uart_serial1_handler) {
    return fd_uart_serial_handler(&fd_uart.devices[1]);
}

ISR_DIRECT_DECLARE(fd_uart_serial2_handler) {
    return fd_uart_serial_handler(&fd_uart.devices[2]);
}

ISR_DIRECT_DECLARE(fd_uart_serial3_handler) {
    return fd_uart_serial_handler(&fd_uart.devices[3]);
}

void fd_uart_initialize(void) {
    memset(&fd_uart, 0, sizeof(fd_uart));
}

void fd_uart_instance_initialize(fd_uart_instance_t *instance) {
    fd_uart_device_t *device = 0;
#ifdef NRF53_SERIES
    if (strcmp(instance->uart_device_name, "NRF_UARTE0_S") == 0) {
        device = &fd_uart.devices[0];
        device->uarte = NRF_UARTE0_S;
        IRQ_DIRECT_CONNECT(SERIAL0_IRQn, 0, fd_uart_serial0_handler, 0);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE1_S") == 0) {
        device = &fd_uart.devices[1];
        device->uarte = NRF_UARTE1_S;
        IRQ_DIRECT_CONNECT(SERIAL1_IRQn, 0, fd_uart_serial1_handler, 0);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE2_S") == 0) {
        device = &fd_uart.devices[2];
        device->uarte = NRF_UARTE2_S;
        IRQ_DIRECT_CONNECT(SERIAL2_IRQn, 0, fd_uart_serial2_handler, 0);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE3_S") == 0) {
        device = &fd_uart.devices[3];
        device->uarte = NRF_UARTE3_S;
        IRQ_DIRECT_CONNECT(SERIAL3_IRQn, 0, fd_uart_serial3_handler, 0);
    }
#endif
    fd_assert(device != 0);
    NRF_UARTE_Type *uarte = device->uarte;

    if (instance->baud_rate < 2400) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud1200;
    } else
    if (instance->baud_rate < 4800) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud2400;
    } else
    if (instance->baud_rate < 9600) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud4800;
    } else
    if (instance->baud_rate < 14400) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud9600;
    } else
    if (instance->baud_rate < 19200) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud14400;
    } else
    if (instance->baud_rate < 28800) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud19200;
    } else
    if (instance->baud_rate < 31250) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud28800;
    } else
    if (instance->baud_rate < 38400) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud31250;
    } else
    if (instance->baud_rate < 56000) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud38400;
    } else
    if (instance->baud_rate < 57600) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud56000;
    } else
    if (instance->baud_rate < 76800) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud57600;
    } else
    if (instance->baud_rate < 115200) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud76800;
    } else
    if (instance->baud_rate < 230400) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud115200;
    } else
    if (instance->baud_rate < 250000) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud230400;
    } else
    if (instance->baud_rate < 460800) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud250000;
    } else
    if (instance->baud_rate < 921600) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud460800;
    } else
    if (instance->baud_rate < 1000000) {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud921600;
    } else {
        uarte->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud1M;
    }

    uarte->CONFIG = 0;
    if (instance->parity == fd_uart_parity_even) {
        uarte->CONFIG |= UARTE_CONFIG_PARITY_Included;
    } else
    if (instance->parity == fd_uart_parity_odd) {
        uarte->CONFIG |= UARTE_CONFIG_PARITYTYPE_Odd | UARTE_CONFIG_PARITY_Included;
    }
    if (instance->stop_bits == fd_uart_stop_bits_2) {
        uarte->CONFIG |= UARTE_CONFIG_STOP_Two;
    }

    fd_gpio_configure_output(instance->tx_gpio, true);
    fd_gpio_configure_input(instance->rx_gpio);

    uarte->TXD.PTR = (uint32_t)device->tx.dma_data;
    uarte->TXD.MAXCNT = 0;
    uarte->RXD.PTR = 0;
    uarte->RXD.MAXCNT = sizeof(device->rx.dma_data) / 2;

    uarte->ENABLE = UARTE_ENABLE_ENABLE_Enabled;

    fd_uart_rx_start(device);
}

fd_uart_device_t *fd_uart_get_device(fd_uart_instance_t *instance) {
    for (int i = 0; i < FD_UART_DEVICE_LIMIT; ++i) {
        fd_uart_device_t *device = &fd_uart.devices[i];
        if (device->instance == instance) {
            return device;
        }
    }
    return 0;
}

size_t fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length) {
    fd_uart_device_t *device = fd_uart_get_device(instance);
    fd_fifo_t *fifo = &device->tx.fifo;
    size_t i;
    for (i = 0; i < length; ++i) {
        if (!fd_fifo_put(fifo, data[i])) {
            break;
        }
    }
    NRF_UARTE_Type *uarte = device->uarte;
    if (!uarte->EVENTS_TXSTARTED || uarte->EVENTS_ENDTX) {
        fd_uart_tx_next_dma_buffer(device);
    }
    return i;
}

void fd_uart_instance_tx_flush(fd_uart_instance_t *instance) {
}

size_t fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length) {
    fd_uart_device_t *device = fd_uart_get_device(instance);
    fd_fifo_t *fifo = &device->rx.fifo;
    uint8_t byte = 0;
    size_t i;
    for (i = 0; i < length; ++i) {
        if (fd_fifo_get(fifo, &byte)) {
            data[i] = byte;
        } else {
            break;
        }
    }
    return i;
}