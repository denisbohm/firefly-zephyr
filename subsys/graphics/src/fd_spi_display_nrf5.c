#include "fd_spi_display.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "soc.h"

#include <hal/nrf_gpio.h>

#include <string.h>

typedef struct {
    struct gpio_dt_spec reset;
    struct gpio_dt_spec cs;
    struct gpio_dt_spec dcn;
    struct gpio_dt_spec sck;
    struct gpio_dt_spec mosi;
    NRF_SPIM_Type *spim;
} fd_spi_display_t;

fd_spi_display_t fd_spi_display;

void fd_spi_display_initialize(void) {
    fd_spi_display.reset = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_spi_display_res), gpios);
    int result = gpio_pin_configure_dt(&fd_spi_display.reset, GPIO_OUTPUT_INACTIVE);
    fd_assert(result == 0);

    fd_spi_display.cs = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_spi_display_cs), gpios);
    result = gpio_pin_configure_dt(&fd_spi_display.cs, GPIO_OUTPUT_INACTIVE);
    fd_assert(result == 0);

    fd_spi_display.dcn = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_spi_display_dcn), gpios);
    result = gpio_pin_configure_dt(&fd_spi_display.dcn, GPIO_OUTPUT_ACTIVE);
    fd_assert(result == 0);

	fd_spi_display.spim = NRF_SPIM2;

	NRF_SPIM_Type *spim = fd_spi_display.spim;
    const uint32_t spi_frequency = 6666666;
#ifdef SPIM_FREQUENCY_FREQUENCY_M32
    if (spi_frequency >= 32000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M32;
    } else
#endif
#ifdef SPIM_FREQUENCY_FREQUENCY_M16
    if (spi_frequency >= 16000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M16;
    } else
#endif
    if (spi_frequency >= 8000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8;
    } else
    if (spi_frequency >= 4000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M4;
    } else
    if (spi_frequency >= 2000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M2;
    } else
    if (spi_frequency >= 1000000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M1;
    } else
    if (spi_frequency >= 500000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K500;
    } else
    if (spi_frequency >= 250000) {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K250;
    } else {
        spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K125;
    }

    const int mode = 0;
    switch (mode) {
        case 0:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);
            break;
        case 1:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
            break;
        case 2:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);
            break;
        case 3:
        default:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
            break;
    }
    
    #define INPUT_PIN(name)	NRF_GPIO_PIN_MAP(DT_PROP(DT_GPIO_CTLR(DT_CHOSEN(name), gpios), port), DT_GPIO_PIN(DT_CHOSEN(name), gpios))

    spim->PSEL.SCK = INPUT_PIN(firefly_spi_display_sck);
    spim->PSEL.MOSI = INPUT_PIN(firefly_spi_display_mosi);
#if DT_NODE_EXISTS(DT_CHOSEN(firefly_spi_display_miso))
    spim->PSEL.MISO = INPUT_PIN(firefly_spi_display_miso);
#endif

    spim->TXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}

void fd_spi_display_reset(void) {
    k_sleep(K_MSEC(25));
    int result = gpio_pin_set_dt(&fd_spi_display.reset, 1);
    fd_assert(result == 0);
    k_sleep(K_MSEC(25));
    result = gpio_pin_set_dt(&fd_spi_display.reset, 0);
    fd_assert(result == 0);
    k_sleep(K_MSEC(120)); // Sleep Out command cannot be sent for 120msec.
}

void fd_spi_display_cs_enable(void) {
    int result = gpio_pin_set_dt(&fd_spi_display.cs, 1);
    fd_assert(result == 0);
}

void fd_spi_display_cs_disable(void) {
    int result = gpio_pin_set_dt(&fd_spi_display.cs, 0);
    fd_assert(result == 0);
}

void fd_spi_display_write_command(const uint8_t *tx_bytes, uint32_t tx_byte_count) {
    uint32_t address = (uint32_t)tx_bytes;
    fd_assert((0x20000000 <= address) && (address <= 0x21000000));
    
    int result = gpio_pin_set_dt(&fd_spi_display.dcn, 0);
    fd_assert(result == 0);

	NRF_SPIM_Type *spim = fd_spi_display.spim;
    spim->TXD.PTR = (uint32_t)tx_bytes;
    spim->TXD.MAXCNT = 1;
    spim->RXD.MAXCNT = 0;
    spim->EVENTS_END = 0;
    spim->TASKS_START = 1;
    while (!spim->EVENTS_END) {
    }

    result = gpio_pin_set_dt(&fd_spi_display.dcn, 1);
    fd_assert(result == 0);

    size_t tx_remaining = tx_byte_count - 1;
    while (tx_remaining > 0) {
        uint32_t tx_amount = tx_remaining > 0xffff ? 0xffff : tx_remaining;
        spim->TXD.MAXCNT = tx_amount;
        spim->EVENTS_END = 0;
        spim->TASKS_START = 1;
        while (!spim->EVENTS_END) {
        }
        tx_remaining -= tx_amount;
    }
}

void fd_spi_display_write_commands(const uint8_t *bytes, size_t size) {
    uint32_t index = 0;
    while (index < size) {
        uint8_t length = bytes[index];
        fd_assert(length >= 1);
        fd_spi_display_write_command(&bytes[index + 1], length);
        index += 1 + length;
    }
}

void fd_spi_display_write_data(const uint8_t *data, size_t size) {
    uint32_t address = (uint32_t)data;
    fd_assert((0x20000000 <= address) && (address <= 0x21000000));
    
	NRF_SPIM_Type *spim = fd_spi_display.spim;
    spim->TXD.PTR = (uint32_t)data;
    spim->RXD.MAXCNT = 0;
    size_t tx_remaining = size;
    while (tx_remaining > 0) {
        uint32_t tx_amount = tx_remaining > 0xffff ? 0xffff : tx_remaining;
        spim->TXD.MAXCNT = tx_amount;
        spim->EVENTS_END = 0;
        spim->TASKS_START = 1;
        while (!spim->EVENTS_END) {
        }
        tx_remaining -= tx_amount;
    }
}

void fd_spi_display_read(uint8_t command, uint8_t *data, size_t size) {
#if DT_NODE_EXISTS(DT_CHOSEN(firefly_spi_display_miso))
    int result = gpio_pin_set_dt(&fd_spi_display.dcn, 0);
    fd_assert(result == 0);

    fd_spi_display_cs_enable();

	NRF_SPIM_Type *spim = fd_spi_display.spim;
    spim->TXD.PTR = (uint32_t)&command;
    spim->TXD.MAXCNT = 1;
    spim->RXD.MAXCNT = 0;
    spim->EVENTS_END = 0;
    spim->TASKS_START = 1;
    while (!spim->EVENTS_END) {
    }

    result = gpio_pin_set_dt(&fd_spi_display.dcn, 1);
    fd_assert(result == 0);

    spim->RXD.PTR = (uint32_t)data;
    spim->TXD.MAXCNT = 0;
    spim->RXD.MAXCNT = size;
    spim->EVENTS_END = 0;
    spim->TASKS_START = 1;
    while (!spim->EVENTS_END) {
    }

    fd_spi_display_cs_disable();
#endif
}