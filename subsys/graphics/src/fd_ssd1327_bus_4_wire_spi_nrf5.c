#include "fd_ssd1327_bus.h"

#include "fd_ssd1327_bus_4_wire_spi_zephyr.h"
#include "fd_gpio.h"
#include "fd_assert.h"

#include <string.h>

#include "nrf.h"

void fd_ssd1327_bus_4_wire_spi_zephyr_configure(
    fd_ssd1327_bus_4_wire_spi_zephyr_configuration_t configuration
) {
}

#define NRF_GPIO_PIN_MAP(port, pin) (port * 32 + pin)

void fd_ssd1327_bus_initialize(void) {
    fd_gpio_t sclk = { .port = 1, .pin = 6 };
    fd_gpio_t mosi = { .port = 1, .pin = 8 };
    fd_gpio_t miso = { .port = 1, .pin = 5 };

    fd_gpio_configure_output(sclk);
    fd_gpio_set(sclk, true);
    fd_gpio_configure_output(mosi);
    fd_gpio_set(mosi, true);
    fd_gpio_configure_input(miso);

    NRF_SPIM_Type *spim = NRF_SPIM2_S;
    spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
    spim->PSEL.SCK = NRF_GPIO_PIN_MAP(sclk.port, sclk.pin);
    spim->PSEL.MOSI = NRF_GPIO_PIN_MAP(mosi.port, mosi.pin);
    spim->PSEL.MISO = NRF_GPIO_PIN_MAP(miso.port, miso.pin);
    spim->TXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->RXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}

void fd_ssd1327_bus_write(const uint8_t *data, uint32_t length) {
    NRF_SPIM_Type *spim = NRF_SPIM2_S;
    spim->TXD.PTR = (uint32_t)data;
    spim->TXD.MAXCNT = length;
    spim->RXD.PTR = 0;
    spim->RXD.MAXCNT = 0;
    spim->EVENTS_END = 0;
    spim->TASKS_START = 1;
    while (!spim->EVENTS_END) {
    }
}
