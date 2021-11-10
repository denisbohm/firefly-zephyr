#include "fd_sdcard_spi_zephyr.h"

#include "fd_assert.h"
#include "fd_gpio.h"

#include <zephyr.h>
#include <drivers/spi.h>

static struct spi_config fd_sdcard_spi_zephyr_config_slow = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .frequency = 400000,
    .slave = 0,
};

static struct spi_config fd_sdcard_spi_zephyr_config_fast = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    .frequency = 20000000,
    .slave = 0,
};

const struct device *fd_sdcard_spi_zephyr_device;
struct spi_config *fd_sdcard_spi_zephyr_config;
fd_gpio_t fd_sdcard_spi_zephyr_csn;

void fd_sdcard_spi_zephyr_set_chip_select(bool value) {
    fd_gpio_set(fd_sdcard_spi_zephyr_csn, value);
}

void fd_sdcard_spi_zephyr_set_frequency_slow(void) {
    fd_sdcard_spi_zephyr_config = &fd_sdcard_spi_zephyr_config_slow;
}

void fd_sdcard_spi_zephyr_set_frequency_fast(void) {
    fd_sdcard_spi_zephyr_config = &fd_sdcard_spi_zephyr_config_fast;
}

void fd_sdcard_spi_zephyr_transceive(const uint8_t *tx_data, size_t tx_size, uint8_t *rx_data, size_t rx_size) {
    const struct spi_buf tx_buf = {
        .buf = (uint8_t *)tx_data,
        .len = tx_size,
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };
    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = rx_size,
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1,
    };
    int result = spi_transceive(fd_sdcard_spi_zephyr_device, fd_sdcard_spi_zephyr_config, &tx, &rx);
    fd_assert(result == 0);
}

void fd_sdcard_spi_zephyr_initialize(void) {
    fd_sdcard_spi_zephyr_device = device_get_binding("SPI_2");
    fd_assert(fd_sdcard_spi_zephyr_device != NULL);

    fd_sdcard_spi_zephyr_csn = (fd_gpio_t) { .port = 0, .pin = 4 };
    fd_gpio_configure_output(fd_sdcard_spi_zephyr_csn, true);

    fd_sdcard_spi_zephyr_config = &fd_sdcard_spi_zephyr_config_slow;
}
