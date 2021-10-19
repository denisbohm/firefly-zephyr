#ifndef fd_sdcard_spi_h
#define fd_sdcard_spi_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void (*set_chip_select)(bool value);
    void (*set_frequency_slow)(void); // <= 400 kHz
    void (*set_frequency_fast)(void); // <= 25 mHz
    void (*transceive)(const uint8_t *tx, size_t tx_count, uint8_t *rx, size_t rx_count);
} fd_sdcard_spi_t;

#endif
