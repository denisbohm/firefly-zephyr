#include "fd_uart.h"

#include "fd_assert.h"
#include "fd_event.h"
#include "fd_fifo.h"

#include <zephyr.h>

#include <nrf.h>
#include <nrfx_uarte.h>

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
    uint32_t event;
    uint8_t dma_data[2];
    uint32_t dma_index;
    uint8_t fifo_data[FD_UART_DEVICE_RX_FIFO_LIMIT];
    fd_fifo_t fifo;
    uint32_t rxstarted_count;
    uint32_t rxrdy_count;
    uint32_t endrx_count;
    uint32_t rxto_count;
} fd_uart_device_rx_t;

typedef struct {
    uint32_t event;
    uint32_t pending;
    uint8_t dma_data[FD_UART_DEVICE_TX_DMA_LIMIT];
    uint8_t fifo_data[FD_UART_DEVICE_TX_FIFO_LIMIT];
    fd_fifo_t fifo;
} fd_uart_device_tx_t;

typedef struct {
    fd_uart_instance_t *instance;
    NRF_UARTE_Type *uarte;
    fd_uart_device_rx_t rx;
    fd_uart_device_tx_t tx;
    uint32_t errorsrc;
} fd_uart_device_t;

typedef struct {
    fd_uart_device_t devices[FD_UART_DEVICE_LIMIT];
} fd_uart_t;

fd_uart_t fd_uart;

size_t fd_uart_tx_set_next_dma_buffer(fd_uart_device_t *device) {
    fd_uart_device_tx_t *tx = &device->tx;
    fd_fifo_t *fifo = &tx->fifo;
    uint8_t byte = 0;
    size_t i;
    for (i = 0; i < sizeof(tx->dma_data); ++i) {
        if (fd_fifo_get(fifo, &byte)) {
            tx->dma_data[i] = byte;
        } else {
            break;
        }
    }
    if (i > 0) {
        NRF_UARTE_Type *uarte = device->uarte;
        tx->pending = i;
        uarte->TXD.MAXCNT = i;
        uarte->TASKS_STARTTX = 1;
    }
    return i;
}

int fd_uart_serial_handler(fd_uart_device_t *device) {
    NRF_UARTE_Type *uarte = device->uarte;

    fd_uart_device_rx_t *rx = &device->rx;
#if 0
    if (uarte->EVENTS_RXDRDY) {
        // generated when a byte is received, but RXD.AMOUNT is not updated yet
        // and the byte may not have been written to RAM by DMA yet
        // So basically, this event appears to be useless...
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXDRDY);
        ++rx->rxrdy_count;
    }
#endif
    if (uarte->EVENTS_ENDRX) {
        // receivied a byte
//        fd_assert(uarte->RXD.AMOUNT == 1);
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDRX);
        uint8_t byte = rx->dma_data[rx->dma_index];
        fd_fifo_put(&rx->fifo, byte);
//        ++rx->endrx_count;
        if (byte == 0) {
            fd_uart_instance_t *instance = device->instance;
            if (instance->isr_rx_callback) {
                instance->isr_rx_callback();
            }
            if (instance->rx_event_name) {
                fd_event_set_from_interrupt(rx->event);
            }
        }
    }
    if (uarte->EVENTS_RXSTARTED) {
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXSTARTED);
        uarte->RXD.PTR = (uint32_t)&rx->dma_data[rx->dma_index];
        rx->dma_index = (rx->dma_index + 1) & 0x1;
//        ++rx->rxstarted_count;
    }
#if 0
    if (uarte->EVENTS_RXTO) {
        // generated when rx stop task is complete
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_RXTO);
        ++rx->rxto_count;
    }
#endif

#if 0
    if (uarte->EVENTS_TXSTARTED) {
        // generated when transmitter has started but no bytes sent yet
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTARTED);
    }
#endif
    if (uarte->EVENTS_TXDRDY) {
        // generated after each byte is transmitted
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXDRDY);
        if (--device->tx.pending == 0) {
            if (fd_uart_tx_set_next_dma_buffer(device) == 0) {
                if (device->instance->isr_tx_callback) {
                    device->instance->isr_tx_callback();
                }
                if (device->instance->tx_event_name) {
                    fd_event_set_from_interrupt(device->tx.event);
                }
            }
        }
    }
#if 0
    if (uarte->EVENTS_ENDTX) {
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ENDTX);
        // ENDTX happens before the last byte is sent.
        // So basically, this event appears to be useless...
        // fd_uart_end_tx(device);
    }
#endif
#if 0
    if (uarte->EVENTS_TXSTOPPED) {
        // generated when tx stop task is complete
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_TXSTOPPED);
    }
#endif

#if 0
    if (uarte->EVENTS_ERROR) {
        device->errorsrc |= uarte->ERRORSRC;
        nrf_uarte_event_clear(uarte, NRF_UARTE_EVENT_ERROR);
    }
#endif

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
        irq_enable(SERIAL0_IRQn);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE1_S") == 0) {
        device = &fd_uart.devices[1];
        device->uarte = NRF_UARTE1_S;
        IRQ_DIRECT_CONNECT(SERIAL1_IRQn, 0, fd_uart_serial1_handler, 0);
        irq_enable(SERIAL1_IRQn);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE2_S") == 0) {
        device = &fd_uart.devices[2];
        device->uarte = NRF_UARTE2_S;
        IRQ_DIRECT_CONNECT(SERIAL2_IRQn, 0, fd_uart_serial2_handler, 0);
        irq_enable(SERIAL2_IRQn);
    } else
    if (strcmp(instance->uart_device_name, "NRF_UARTE3_S") == 0) {
        device = &fd_uart.devices[3];
        device->uarte = NRF_UARTE3_S;
        IRQ_DIRECT_CONNECT(SERIAL3_IRQn, 0, fd_uart_serial3_handler, 0);
        irq_enable(SERIAL3_IRQn);
    }
#endif
    fd_assert(device != 0);

    device->instance = instance;
    fd_fifo_initialize(&device->tx.fifo, device->tx.fifo_data, sizeof(device->tx.fifo_data));
    fd_fifo_initialize(&device->rx.fifo, device->rx.fifo_data, sizeof(device->rx.fifo_data));

    if (instance->tx_event_name) {
        device->tx.event = fd_event_get_identifier(instance->tx_event_name);
    }
    if (instance->rx_event_name) {
        device->rx.event = fd_event_get_identifier(instance->rx_event_name);
    }

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
        uarte->CONFIG |= UARTE_CONFIG_PARITY_Included << UARTE_CONFIG_PARITY_Pos;
    } else
    if (instance->parity == fd_uart_parity_odd) {
#ifdef NRF53_SERIES
        uarte->CONFIG |= (UARTE_CONFIG_PARITYTYPE_Odd | UARTE_CONFIG_PARITY_Included) << UARTE_CONFIG_PARITY_Pos;
#endif
#ifdef NRF52_SERIES
        uarte->CONFIG |= UARTE_CONFIG_PARITY_Included << UARTE_CONFIG_PARITY_Pos;
#endif
    }
    if (instance->stop_bits == fd_uart_stop_bits_2) {
        uarte->CONFIG |= UARTE_CONFIG_STOP_Two << UARTE_CONFIG_STOP_Pos;
    }

    fd_gpio_configure_output(instance->tx_gpio, true);
    fd_gpio_configure_input(instance->rx_gpio);
    uarte->PSEL.TXD = instance->tx_gpio.port * 32 + instance->tx_gpio.pin;
    uarte->PSEL.RXD = instance->rx_gpio.port * 32 + instance->rx_gpio.pin;

    uarte->TXD.PTR = (uint32_t)device->tx.dma_data;
    uarte->TXD.MAXCNT = 0;
    uarte->RXD.PTR = 0;
    uarte->RXD.MAXCNT = sizeof(device->rx.dma_data) / 2;
    uarte->SHORTS = UARTE_SHORTS_ENDRX_STARTRX_Msk;

    uarte->INTEN =
        UARTE_INTEN_RXSTARTED_Msk | UARTE_INTEN_ENDRX_Msk |
//        UARTE_INTEN_RXSTARTED_Msk | /* UARTE_INTEN_RXDRDY_Msk | */ UARTE_INTEN_ENDRX_Msk | UARTE_INTEN_RXTO_Msk |
        UARTE_INTEN_TXDRDY_Msk;
//        UARTE_INTEN_TXSTARTED_Msk | UARTE_INTEN_TXDRDY_Msk | UARTE_INTEN_ENDTX_Msk | UARTE_INTEN_TXSTOPPED_Msk;
    uarte->ENABLE = UARTE_ENABLE_ENABLE_Enabled;

    uarte->TASKS_STARTRX = 1;
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
    if (device->tx.pending == 0) {
        fd_uart_tx_set_next_dma_buffer(device);
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