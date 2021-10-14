#ifndef fd_sdcard_spi_zephyr_h

#include "fd_sdcard_spi.h"

void fd_sdcard_spi_zephyr_initialize(void);

void fd_sdcard_spi_zephyr_set_chip_select(bool value);
void fd_sdcard_spi_zephyr_set_frequency_slow(void);
void fd_sdcard_spi_zephyr_set_frequency_fast(void);
void fd_sdcard_spi_zephyr_transceive(const uint8_t *tx_data, size_t tx_size, uint8_t *rx_data, size_t rx_size);

#endif
