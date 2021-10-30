#ifndef fd_ssd1327_bus_4_wire_spi_h
#define fd_ssd1327_bus_4_wire_spi_h

#include "fd_gpio.h"

typedef struct {
    const char *spi_device_name;
    fd_gpio_t sclk;
    fd_gpio_t mosi;
    fd_gpio_t miso;
} fd_ssd1327_bus_4_wire_spi_configuration_t;

void fd_ssd1327_bus_4_wire_spi_configure(fd_ssd1327_bus_4_wire_spi_configuration_t configuration);

#endif
