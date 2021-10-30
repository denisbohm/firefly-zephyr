#include "fd_ssd1327_bus.h"

#include "fd_ssd1327_bus_4_wire_spi.h"
#include "fd_assert.h"

#include <drivers/spi.h>

#include <string.h>

// 4-wire SPI interface

fd_ssd1327_bus_4_wire_spi_configuration_t fd_ssd1327_bus_4_wire_spi_configuration = {
    .spi_device_name = "SPI_1",
};

typedef struct {
    const struct device *spi_device;
} fd_ssd1327_bus_4_wire_spi_t;

fd_ssd1327_bus_4_wire_spi_t fd_ssd1327_bus_4_wire_spi;

static const struct spi_config fd_ssd1327_bus_4_wire_spi_config = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .frequency = 10000000,
    .slave = 0,
};

void fd_ssd1327_bus_write(const uint8_t *data, uint32_t length) {
    const struct spi_buf tx_buf = {
        .buf = (uint8_t *)data,
        .len = length,
    };
    const struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1,
    };
    spi_write(fd_ssd1327_bus_4_wire_spi.spi_device, &fd_ssd1327_bus_4_wire_spi_config, &tx_bufs);
}

void fd_ssd1327_bus_4_wire_spi_configure(
    fd_ssd1327_bus_4_wire_spi_configuration_t configuration
) {
    fd_ssd1327_bus_4_wire_spi_configuration = configuration;
}

void fd_ssd1327_bus_initialize(void) {
    memset(&fd_ssd1327_bus_4_wire_spi, 0, sizeof(fd_ssd1327_bus_4_wire_spi));

    fd_ssd1327_bus_4_wire_spi.spi_device = device_get_binding(
        fd_ssd1327_bus_4_wire_spi_configuration.spi_device_name
    );
    fd_assert(fd_ssd1327_bus_4_wire_spi.spi_device != NULL);
}
