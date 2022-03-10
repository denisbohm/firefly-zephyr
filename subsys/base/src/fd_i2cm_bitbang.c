#include "fd_i2cm_bitbang.h"

#include "fd_delay.h"
#include "fd_gpio.h"

void fd_i2cm_bitbang_delay(void) {
    fd_delay_us(1);
}

void fd_i2cm_bitbang_configure_in(const fd_i2cm_bus_t *bus) {
    if (bus->pullup) {
        fd_gpio_configure_input_pull_up(bus->sda);
    } else {
        fd_gpio_configure_input(bus->sda);
    }
}

void fd_i2cm_bitbang_configure_out(const fd_i2cm_bus_t *bus) {
    if (bus->pullup) {
        fd_gpio_configure_output_open_drain_pull_up(bus->sda, true);
    } else {
        fd_gpio_configure_output_open_drain(bus->sda, true);
    }
}

void fd_i2cm_bitbang_start(const fd_i2cm_bus_t *bus) {
    fd_gpio_set(bus->scl, true);
    fd_gpio_set(bus->sda, true);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->sda, false);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->scl, false);
    fd_i2cm_bitbang_delay();
}

void fd_i2cm_bitbang_stop(const fd_i2cm_bus_t *bus) {
    fd_gpio_set(bus->sda, false);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->scl, true);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->sda, true);
    fd_i2cm_bitbang_delay();
}

void fd_i2cm_bitbang_write_bit(const fd_i2cm_bus_t *bus, bool bit) {
    fd_gpio_set(bus->sda, bit);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->scl, true);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->scl, false);
}

bool fd_i2cm_bitbang_read_bit(const fd_i2cm_bus_t *bus) {
    fd_gpio_set(bus->sda, true);
    fd_i2cm_bitbang_delay();
    fd_gpio_set(bus->scl, true);
    fd_i2cm_bitbang_delay();
    bool bit = fd_gpio_get(bus->sda);
    fd_gpio_set(bus->scl, false);
    return bit;
}

bool fd_i2cm_bitbang_write_byte(const fd_i2cm_bus_t *bus, uint8_t byte) {
    for (int i = 0; i < 8; ++i) {
        fd_i2cm_bitbang_write_bit(bus, (byte & 0x80) != 0);
        byte <<= 1;
    }

    fd_gpio_set(bus->sda, true);
    fd_i2cm_bitbang_configure_in(bus);
    bool ack = fd_i2cm_bitbang_read_bit(bus);
    fd_i2cm_bitbang_configure_out(bus);
    return !ack;
}

uint8_t fd_i2cm_bitbang_read_byte(const fd_i2cm_bus_t *bus, bool ack) {
    fd_i2cm_bitbang_configure_in(bus);
    uint8_t byte = 0;
    for (int i = 0; i < 8; ++i) {
        bool bit = fd_i2cm_bitbang_read_bit(bus);
        byte = (byte << 1) | (bit ? 1 : 0);
    }
    fd_i2cm_bitbang_configure_out(bus);
    fd_i2cm_bitbang_write_bit(bus, !ack);
    return byte;
}

bool fd_i2cm_bitbang_bus_tx(const fd_i2cm_bus_t *bus, const uint8_t *bytes, uint32_t count) {
    for (int i = 0; i < count; ++i) {
        bool ack = fd_i2cm_bitbang_write_byte(bus, bytes[i]);
        if (!ack) {
            return false;
        }
    }
    return true;
}

void fd_i2cm_bitbang_bus_rx(const fd_i2cm_bus_t *bus, uint8_t *bytes, uint32_t count, bool ack) {
    for (int i = 0; i < count; ++i) {
        bytes[i] = fd_i2cm_bitbang_read_byte(bus, ack || (i < (count - 1)));
    }
}

bool fd_i2cm_bitbang_start_write(const fd_i2cm_device_t *device) {
    fd_i2cm_bitbang_start(device->bus);
    return fd_i2cm_bitbang_write_byte(device->bus, device->address << 1);
}

bool fd_i2cm_bitbang_start_read(const fd_i2cm_device_t *device) {
    fd_i2cm_bitbang_start(device->bus);
    return fd_i2cm_bitbang_write_byte(device->bus, (device->address << 1) | 1);
}

bool fd_i2cm_bitbang_device_io(const fd_i2cm_device_t *device, const fd_i2cm_io_t *io) {
    int last_direction = -1;
    bool ack = true;
    for (int i = 0; i < io->transfer_count; ++i) {
        const fd_i2cm_transfer_t *transfer = &io->transfers[i];
        switch (transfer->direction) {
            case fd_i2cm_direction_tx:
                if (transfer->direction != last_direction) {
                    ack = fd_i2cm_bitbang_start_write(device);
                    if (!ack) {
                        goto stop;
                    }
                }
                ack = fd_i2cm_bitbang_bus_tx(device->bus, transfer->bytes, transfer->byte_count);
                if (!ack) {
                    goto stop;
                }
            break;
            case fd_i2cm_direction_rx:
                if (transfer->direction != last_direction) {
                    ack = fd_i2cm_bitbang_start_read(device);
                    if (!ack) {
                        goto stop;
                    }
                }
                bool rx_ack =
                    (i < io->transfer_count - 1) &&
                    (io->transfers[i + 1].direction == fd_i2cm_direction_rx);
                fd_i2cm_bitbang_bus_rx(device->bus, transfer->bytes, transfer->byte_count, rx_ack);
            break;
            default:
            break;
        }
        last_direction = transfer->direction;
    }
    
stop:
    fd_i2cm_bitbang_stop(device->bus);
    return ack;
}