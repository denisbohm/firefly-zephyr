#ifndef fd_spi_display_h
#define fd_spi_display_h

#include <stddef.h>
#include <stdint.h>

void fd_spi_display_initialize(void);

void fd_spi_display_reset(void);

void fd_spi_display_cs_enable(void);
void fd_spi_display_cs_disable(void);

#define fd_spi_display_command(command, ...) (1 __VA_OPT__(+ sizeof((uint8_t[]){__VA_ARGS__}))), command __VA_OPT__(,) __VA_ARGS__
void fd_spi_display_write_commands(const uint8_t *commands, size_t size);

void fd_spi_display_write_data(const uint8_t *data, size_t size);

#endif