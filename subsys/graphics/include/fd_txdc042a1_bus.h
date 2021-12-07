#ifndef fd_txdc042a1_bus_h
#define fd_txdc042a1_bus_h

#include <stdint.h>

typedef struct {
    const char *spi_device_name;
} fd_txdc042a1_bus_4_wire_spi_zephyr_configuration_t;

void fd_txdc042a1_bus_initialize(void);
void fd_txdc042a1_bus_write(const uint8_t *data, uint32_t length);
void fd_txdc042a1_bus_write_pixel(const uint8_t *data, uint32_t length);

#endif
