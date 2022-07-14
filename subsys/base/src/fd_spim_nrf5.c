#include "fd_spim.h"

#include "fd_assert.h"
#include "fd_delay.h"

#include "nrf.h"
#include "hal/nrf_gpio.h"

#include <string.h>

typedef struct {
    const fd_spim_bus_t *buses;
    uint32_t bus_count;
    const fd_spim_device_t *devices;
    uint32_t device_count;
} fd_spim_t;

fd_spim_t fd_spim;

static inline
NRF_GPIO_Type *fd_spim_get_nrf_gpio(uint32_t port) {
#ifdef NRF52_SERIES
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
#endif
#ifdef NRF53_SERIES
    return (NRF_GPIO_Type *)(NRF_P0_S_BASE + port * 0x300UL);
#endif
}

static
void fd_spim_configure(
    fd_gpio_t gpio,
    nrf_gpio_pin_dir_t dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense
) {
    NRF_GPIO_Type *nrf_gpio = fd_spim_get_nrf_gpio(gpio.port);
    nrf_gpio->PIN_CNF[gpio.pin] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                                | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                                | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                                | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                                | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}

void fd_spim_configure_output(fd_gpio_t gpio) {
    fd_spim_configure(gpio,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_spim_initialize(
    const fd_spim_bus_t *buses, uint32_t bus_count,
    const fd_spim_device_t *devices, uint32_t device_count
) {
    memset(&fd_spim, 0, sizeof(fd_spim));
    fd_spim.buses = buses;
    fd_spim.bus_count = bus_count;
    fd_spim.devices = devices;
    fd_spim.device_count = device_count;

    for (uint32_t i = 0; i < device_count; ++i) {
        const fd_spim_device_t *device = &devices[i];
        if (device->bus->power_off) {
            fd_gpio_configure_default(device->csn);
        } else {
            fd_gpio_configure_output(device->csn, true);
        }
    }
    for (uint32_t i = 0; i < bus_count; ++i) {
        const fd_spim_bus_t *bus = &buses[i];
        if (bus->power_off) {
            fd_gpio_configure_default(bus->sclk);
            fd_gpio_configure_default(bus->mosi);
            fd_gpio_configure_default(bus->miso);
        } else {
            fd_spim_configure_output(bus->sclk);
            fd_gpio_set(bus->sclk, false);
            fd_spim_configure_output(bus->mosi);
            fd_gpio_set(bus->mosi, false);
            fd_gpio_configure_default(bus->miso);
        }

        fd_spim_bus_disable(bus);
    }
}

const fd_spim_bus_t *fd_spim_get_bus(int index) {
    if (index >= fd_spim.bus_count) {
        return 0;
    }
    return &fd_spim.buses[index];
}

const fd_spim_device_t *fd_spim_get_device(int index) {
    if (index >= fd_spim.device_count) {
        return 0;
    }
    return &fd_spim.devices[index];
}

void fd_spim_bus_enable(const fd_spim_bus_t *bus) {
    if (fd_spim_bus_is_enabled(bus)) {
        return;
    }

    if (bus->power_off) {
        for (uint32_t i = 0; i < fd_spim.device_count; ++i) {
            const fd_spim_device_t *device = &fd_spim.devices[i];
            if (device->bus == bus) {
                fd_spim_configure_output(device->csn);
                fd_gpio_set(device->csn, true);
            }
        }
    }

    fd_spim_configure_output(bus->sclk);
    fd_gpio_set(bus->sclk, false);
    fd_spim_configure_output(bus->mosi);
    fd_gpio_set(bus->mosi, false);
    fd_gpio_configure_input_pull_up(bus->miso);

    if (bus->instance == 0) {
        return;
    }

    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    switch (bus->frequency) {
        default:
        case 8000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8;
            break;
#ifdef SPIM_FREQUENCY_FREQUENCY_M16
        case 16000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M16;
            break;
#endif
#ifdef SPIM_FREQUENCY_FREQUENCY_M32
        case 32000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M32;
            break;
#endif
    }
    switch (bus->mode) {
        case fd_spim_mode_0:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);
            break;
        case fd_spim_mode_1:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
            break;
        case fd_spim_mode_2:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);
            break;
        case fd_spim_mode_3:
        default:
            spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
            break;
    }
    spim->PSEL.SCK = NRF_GPIO_PIN_MAP(bus->sclk.port, bus->sclk.pin);
    spim->PSEL.MOSI = NRF_GPIO_PIN_MAP(bus->mosi.port, bus->mosi.pin);
    spim->PSEL.MISO = NRF_GPIO_PIN_MAP(bus->miso.port, bus->miso.pin);
    spim->TXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->RXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}

void fd_spim_bus_disable(const fd_spim_bus_t *bus) {
    if (!fd_spim_bus_is_enabled(bus)) {
        return;
    }

    if (bus->power_off) {
        fd_gpio_configure_default(bus->sclk);
        fd_gpio_configure_default(bus->mosi);
        fd_gpio_configure_default(bus->miso);
        for (uint32_t i = 0; i < fd_spim.device_count; ++i) {
            const fd_spim_device_t *device = &fd_spim.devices[i];
            if (device->bus == bus) {
                fd_gpio_configure_default(device->csn);
            }
        }
    } else {
        fd_spim_configure_output(bus->sclk);
        fd_gpio_set(bus->sclk, false);
        fd_spim_configure_output(bus->mosi);
        fd_gpio_set(bus->mosi, false);
        fd_gpio_configure_default(bus->miso);
    }

    if (bus->instance == 0) {
        return;
    }
    
    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    spim->INTENCLR = SPIM_INTENCLR_END_Msk;
    spim->EVENTS_STARTED = 0;
    spim->EVENTS_STOPPED = 0;
    spim->EVENTS_ENDRX = 0;
    spim->EVENTS_ENDTX = 0;
    spim->EVENTS_END = 0;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;

#ifdef NRF52840_XXAA
    if (spim == NRF_SPIM3) {
        // [195] SPIM: SPIM3 continues to draw current after disable
        *(volatile uint32_t *)0x4002F004 = 1;
    }
#endif

    spim->PSEL.SCK = 0xFFFFFFFF;
    spim->PSEL.MOSI = 0xFFFFFFFF;
    spim->PSEL.MISO = 0xFFFFFFFF;
}

bool fd_spim_bus_is_enabled(const fd_spim_bus_t *bus) {
    if (bus->instance == 0) {
        NRF_GPIO_Type *nrf_gpio = fd_spim_get_nrf_gpio(bus->miso.port);
        return ((nrf_gpio->PIN_CNF[bus->miso.pin] >> GPIO_PIN_CNF_INPUT_Pos) & 1) == 0;
    }

    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    const uint32_t mask = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    return (spim->ENABLE & mask) == mask;
}

void fd_spim_device_select(const fd_spim_device_t *device) {
    fd_assert(fd_spim_bus_is_enabled(device->bus));
    fd_gpio_set(device->csn, false);
}

void fd_spim_device_deselect(const fd_spim_device_t *device) {
    fd_assert(fd_spim_bus_is_enabled(device->bus));
    fd_gpio_set(device->csn, true);
}

bool fd_spim_device_is_selected(const fd_spim_device_t *device) {
    return !fd_gpio_get(device->csn);
}

static inline
NRF_GPIO_Type *fd_spim_transfer_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
}

uint8_t fd_spim_transfer_byte(const fd_spim_bus_t *bus, uint8_t tx) {
    // mode 1: CPOL=0, CPHA=1

    NRF_GPIO_Type *sclk_gpio = fd_spim_transfer_get_nrf_gpio(bus->sclk.port);
    uint32_t sclk_mask = 1UL << bus->sclk.pin;

    NRF_GPIO_Type *mosi_gpio = fd_spim_transfer_get_nrf_gpio(bus->mosi.port);
    uint32_t mosi_mask = 1UL << bus->mosi.pin;

    NRF_GPIO_Type *miso_gpio = fd_spim_transfer_get_nrf_gpio(bus->miso.port);
    uint32_t miso_mask = 1UL << bus->miso.pin;

    uint8_t rx = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        // fd_gpio_set(bus->mosi, (tx & 0x80) != 0);
        if (tx & 0x80) {
            mosi_gpio->OUTSET = mosi_mask;
        } else {
            mosi_gpio->OUTCLR = mosi_mask;
        }
        tx <<= 1;

        // fd_gpio_set(bus->sclk, true);
        sclk_gpio->OUTSET = sclk_mask;
        // fd_delay_us(1);
        __asm("nop\nnop\nnop");

        // rx = (rx << 1) | (fd_gpio_get(bus->miso) ? 1 : 0);
        rx <<= 1;
        if (miso_gpio->IN & miso_mask) {
            rx |= 1;
        }

        // fd_gpio_set(bus->sclk, false);
        sclk_gpio->OUTCLR = sclk_mask;
        //fd_delay_us(1);
        __asm("nop\nnop\nnop");
    }
    return rx;
}

void fd_spim_transfer_bit_bang(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    fd_assert(fd_spim_bus_is_enabled(bus));

    NRF_GPIO_Type *sclk_gpio = fd_spim_transfer_get_nrf_gpio(bus->sclk.port);
    uint32_t sclk_mask = 1UL << bus->sclk.pin;

    NRF_GPIO_Type *mosi_gpio = fd_spim_transfer_get_nrf_gpio(bus->mosi.port);
    uint32_t mosi_mask = 1UL << bus->mosi.pin;

    NRF_GPIO_Type *miso_gpio = fd_spim_transfer_get_nrf_gpio(bus->miso.port);
    uint32_t miso_mask = 1UL << bus->miso.pin;

    //fd_gpio_set(bus->sclk, false);
    sclk_gpio->OUTCLR = sclk_mask;

    const uint8_t *tx = tx_bytes;
    uint8_t *rx = rx_bytes;
    size_t tx_remaining = tx_byte_count;
    size_t rx_remaining = rx_byte_count;
    while ((tx_remaining > 0) || (rx_remaining > 0)) {
        uint8_t tx_byte = 0;
        if (tx_remaining > 0) {
            tx_byte = *tx++;
            --tx_remaining;
        }

        //uint8_t rx_byte = fd_spim_transfer_byte(bus, tx_byte);
        uint8_t rx_byte = 0;
        for (uint32_t j = 0; j < 8; ++j) {
            // fd_gpio_set(bus->mosi, (tx & 0x80) != 0);
            if (tx_byte & 0x80) {
                mosi_gpio->OUTSET = mosi_mask;
            } else {
                mosi_gpio->OUTCLR = mosi_mask;
            }
            tx_byte <<= 1;

            // fd_gpio_set(bus->sclk, true);
            sclk_gpio->OUTSET = sclk_mask;
            // fd_delay_us(1);
            __asm("nop\nnop\nnop");

            // rx = (rx << 1) | (fd_gpio_get(bus->miso) ? 1 : 0);
            rx_byte <<= 1;
            if (miso_gpio->IN & miso_mask) {
                rx_byte |= 1;
            }

            // fd_gpio_set(bus->sclk, false);
            sclk_gpio->OUTCLR = sclk_mask;
            //fd_delay_us(1);
            __asm("nop\nnop\nnop");
        }

        if (rx_remaining > 0) {
            *rx++ = rx_byte;
            --rx_remaining;
        }
    }
    fd_gpio_set(bus->mosi, false);
}

void fd_spim_transfer_peripheral(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    fd_assert(fd_spim_bus_is_enabled(bus));
    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    spim->TXD.PTR = (uint32_t)tx_bytes;
    spim->RXD.PTR = (uint32_t)rx_bytes;
    size_t tx_remaining = tx_byte_count;
    size_t rx_remaining = rx_byte_count;
    while ((tx_remaining > 0) || (rx_remaining > 0)) {
        uint32_t tx_amount = tx_remaining > 0xff ? 0xff : tx_remaining;
        spim->TXD.MAXCNT = tx_amount;
        uint32_t rx_amount = rx_remaining > 0xff ? 0xff : rx_remaining;
        spim->RXD.MAXCNT = rx_amount;
        spim->EVENTS_END = 0;
        spim->TASKS_START = 1;
        while (!spim->EVENTS_END) {
        }
        tx_remaining -= tx_amount;
        rx_remaining -= rx_amount;
    }
}

void fd_spim_transfer(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    if (
        (bus->instance == 0) ||
        ((tx_byte_count <= 1) && (rx_byte_count <= 1))
    ) {
        fd_spim_transfer_bit_bang(bus, tx_bytes, tx_byte_count, rx_bytes, rx_byte_count);
    } else {
        fd_spim_transfer_peripheral(bus, tx_bytes, tx_byte_count, rx_bytes, rx_byte_count);
    }
}

void fd_spim_bus_io(const fd_spim_bus_t *bus, const fd_spim_io_t *io) {
    for (uint32_t i = 0; i < io->transfer_count; ++i) {
        fd_spim_transfer_t *transfer = &io->transfers[i];
        fd_spim_transfer(bus, transfer->tx_bytes, transfer->tx_byte_count, transfer->rx_bytes, transfer->rx_byte_count);
    }
}

void fd_spim_bus_wait(const fd_spim_bus_t *bus) {
    fd_assert(fd_spim_bus_is_enabled(bus));
}