#ifndef fd_txdc042a1_bus_4_wire_spi_h
#define fd_txdc042a1_bus_4_wire_spi_h

#include "fd_gpio.h"

typedef struct {
    const char *spi_device_name;
    fd_gpio_t sclk;
    fd_gpio_t mosi;
} fd_txdc042a1_bus_4_wire_spi_configuration_t;

void fd_txdc042a1_bus_4_wire_spi_configure(fd_txdc042a1_bus_4_wire_spi_configuration_t configuration);

#endif
