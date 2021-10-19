#ifndef fd_ssd1327_bus_4_wire_spi_zephyr_h
#define fd_ssd1327_bus_4_wire_spi_zephyr_h

typedef struct {
    const char *spi_device_name;
} fd_ssd1327_bus_4_wire_spi_zephyr_configuration_t;

void fd_ssd1327_bus_4_wire_spi_zephyr_configure(fd_ssd1327_bus_4_wire_spi_zephyr_configuration_t configuration);

#endif
