#ifndef fd_ili9341_bus_h
#define fd_ili9341_bus_h

#include <stdint.h>

void fd_ili9341_bus_initialize(void);
void fd_ili9341_bus_read(uint8_t *data, uint32_t length);
void fd_ili9341_bus_write(const uint8_t *data, uint32_t length);
void fd_ili9341_bus_write_pixel(const uint8_t *data, uint32_t length);

#endif
