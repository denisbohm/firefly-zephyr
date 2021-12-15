#include "fd_txdc042a1_bus.h"

#include "fd_txdc042a1_bus_4_wire_spi.h"
#include "fd_gpio.h"
#include "fd_assert.h"

#include <string.h>

#include "nrf.h"

fd_txdc042a1_bus_4_wire_spi_configuration_t fd_txdc042a1_bus_4_wire_spi_configuration = {
    .spi_device_name = "NRF_SPIM1",
    .sclk = { .port = 0, .pin = 21 },
    .mosi = { .port = 0, .pin = 20 },
};

void fd_txdc042a1_bus_4_wire_spi_configure(
    fd_txdc042a1_bus_4_wire_spi_configuration_t configuration
) {
    fd_txdc042a1_bus_4_wire_spi_configuration = configuration;
}

typedef struct {
    NRF_SPIM_Type *spim;
} fd_txdc042a1_bus_4_wire_spi_t;

fd_txdc042a1_bus_4_wire_spi_t fd_txdc042a1_bus_4_wire_spi;

#define NRF_GPIO_PIN_MAP(port, pin) (port * 32 + pin)

void fd_txdc042a1_bus_initialize(void) {
    memset(&fd_txdc042a1_bus_4_wire_spi, 0, sizeof(fd_txdc042a1_bus_4_wire_spi));

    fd_gpio_t sclk = fd_txdc042a1_bus_4_wire_spi_configuration.sclk;
    fd_gpio_t mosi = fd_txdc042a1_bus_4_wire_spi_configuration.mosi;

    fd_gpio_configure_output(sclk, true);
    fd_gpio_configure_output(mosi, true);

    NRF_SPIM_Type *spim = 0;
    const char *name = fd_txdc042a1_bus_4_wire_spi_configuration.spi_device_name;
#ifdef NRF52_SERIES
    if (strcmp(name, "NRF_SPIM0") == 0) {
        spim = NRF_SPIM0;
    } else
    if (strcmp(name, "NRF_SPIM1") == 0) {
        spim = NRF_SPIM1;
    } else
    if (strcmp(name, "NRF_SPIM2") == 0) {
        spim = NRF_SPIM2;
    } else
    if (strcmp(name, "NRF_SPIM3") == 0) {
        spim = NRF_SPIM3;
    }
#endif
#ifdef NRF53_SERIES
    if (strcmp(name, "NRF_SPIM0_S") == 0) {
        spim = NRF_SPIM0_S;
    } else
    if (strcmp(name, "NRF_SPIM1_S") == 0) {
        spim = NRF_SPIM1_S;
    } else
    if (strcmp(name, "NRF_SPIM2_S") == 0) {
        spim = NRF_SPIM2_S;
    } else
    if (strcmp(name, "NRF_SPIM3_S") == 0) {
        spim = NRF_SPIM3_S;
    } else
    if (strcmp(name, "NRF_SPIM4_S") == 0) {
        spim = NRF_SPIM4_S;
    }
#endif
    fd_assert(spim != 0);
    fd_txdc042a1_bus_4_wire_spi.spim = spim;
    spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
    spim->PSEL.SCK = NRF_GPIO_PIN_MAP(sclk.port, sclk.pin);
    spim->PSEL.MOSI = NRF_GPIO_PIN_MAP(mosi.port, mosi.pin);
// txdc042a1 does not have MOSI -denis
//    spim->PSEL.MISO = NRF_GPIO_PIN_MAP(miso.port, miso.pin);
    spim->TXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->RXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}

void fd_txdc042a1_bus_write(const uint8_t *data, uint32_t length) {
    NRF_SPIM_Type *spim = fd_txdc042a1_bus_4_wire_spi.spim;
    spim->TXD.PTR = (uint32_t)data;
    spim->TXD.MAXCNT = length;
    spim->RXD.PTR = 0;
    spim->RXD.MAXCNT = 0;
    spim->EVENTS_END = 0;
    spim->TASKS_START = 1;
    while (!spim->EVENTS_END) {
    }
}
