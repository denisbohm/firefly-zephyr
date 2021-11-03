#include "fd_i2cm.h"

#include "fd_delay.h"
#include "fd_i2cm_bitbang.h"

#include "nrf.h"

#include <string.h>

#ifndef FD_I2CM_BUS_LIMIT
#define FD_I2CM_BUS_LIMIT 8
#endif

typedef struct {
    bool enabled;
    NRF_TWIM_Type *twim;
} fd_i2cm_bus_info_t;

typedef struct {
    const fd_i2cm_bus_t *buses;
    uint32_t bus_count;
    const fd_i2cm_device_t *devices;
    uint32_t device_count;
    fd_i2cm_bus_info_t bus_info[FD_I2CM_BUS_LIMIT];
} fd_i2cm_t;

fd_i2cm_t fd_i2cm;

#define NRF_GPIO_PIN_MAP(port, pin) (port * 32 + pin)

void fd_i2cm_clear_bus(const fd_i2cm_bus_t *bus) {
    bool enabled = fd_i2cm_bus_is_enabled(bus);
    if (enabled) {
        fd_i2cm_bus_disable(bus);
    }

    fd_gpio_configure_default(bus->sda);

    fd_gpio_configure_output_open_drain(bus->scl);
    fd_gpio_set(bus->scl, true);
    fd_delay_us(4);
    for (int i = 0; i < 9; i++) {
        fd_gpio_set(bus->scl, false);
        fd_delay_us(4);
        fd_gpio_set(bus->scl, true);
        fd_delay_us(4);
    }
    fd_gpio_configure_default(bus->scl);

    if (enabled) {
        fd_i2cm_bus_enable(bus);
    }
}

void fd_i2cm_initialize(
    const fd_i2cm_bus_t *buses, uint32_t bus_count,
    const fd_i2cm_device_t *devices, uint32_t device_count
) {
    memset(&fd_i2cm, 0, sizeof(fd_i2cm));
    fd_i2cm.buses = buses;
    fd_i2cm.bus_count = bus_count;
    fd_i2cm.devices = devices;
    fd_i2cm.device_count = device_count;
    for (uint32_t i = 0; i < bus_count; ++i) {
        const fd_i2cm_bus_t *bus = &buses[i];
        fd_i2cm_bus_disable(bus);
        fd_i2cm_clear_bus(bus);
        fd_i2cm_bus_info_t *info = &fd_i2cm.bus_info[i];
        const char *name = bus->i2c_device_name;
        if (name == 0) {
            info->twim = 0;
        } else
        if (strcmp(name, "NRF_TWIM0_S") == 0) {
            info->twim = NRF_TWIM0_S;
        } else
        if (strcmp(name, "NRF_TWIM1_S") == 0) {
            info->twim = NRF_TWIM1_S;
        } else
        if (strcmp(name, "NRF_TWIM2_S") == 0) {
            info->twim = NRF_TWIM2_S;
        } else
        if (strcmp(name, "NRF_TWIM3_S") == 0) {
            info->twim = NRF_TWIM3_S;
        }
    }
}

fd_i2cm_bus_info_t *fd_i2cm_get_bus_info(const fd_i2cm_bus_t *bus) {
    for (int i = 0; i < fd_i2cm.bus_count; ++i) {
        if (bus == &fd_i2cm.buses[i]) {
            return &fd_i2cm.bus_info[i];
        }
    }
    return 0;
}

void fd_i2cm_bus_enable(const fd_i2cm_bus_t *bus) {
    if (fd_i2cm_bus_is_enabled(bus)) {
        return;
    }

    fd_gpio_set(bus->scl, true);
    if (bus->pullup) {
        fd_gpio_configure_output_open_drain_pull_up(bus->scl);
    } else {
        fd_gpio_configure_output_open_drain(bus->scl);
    }
    fd_gpio_set(bus->sda, true);
    if (bus->pullup) {
        fd_gpio_configure_output_open_drain_pull_up(bus->sda);
    } else {
        fd_gpio_configure_output_open_drain(bus->sda);
    }

    fd_i2cm_bus_info_t *bus_info = fd_i2cm_get_bus_info(bus);
    bus_info->enabled = true;
    NRF_TWIM_Type *twim = bus_info->twim;
    if (twim == 0) {
        return;
    }
 
    twim->PSEL.SCL = NRF_GPIO_PIN_MAP(bus->scl.port, bus->scl.pin);
    twim->PSEL.SDA = NRF_GPIO_PIN_MAP(bus->sda.port, bus->sda.pin);

    if (bus->frequency == 400000) {
        twim->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K400 << TWIM_FREQUENCY_FREQUENCY_Pos;
    } else
    if (bus->frequency == 250000) {
        twim->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K250 << TWIM_FREQUENCY_FREQUENCY_Pos;
    } else {
        twim->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K100 << TWIM_FREQUENCY_FREQUENCY_Pos;
    }
    twim->SHORTS = 0;

    twim->ENABLE = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;
}

void fd_i2cm_bus_disable(const fd_i2cm_bus_t *bus) {
    if (!fd_i2cm_bus_is_enabled(bus)) {
        return;
    }

    fd_gpio_configure_default(bus->scl);
    fd_gpio_configure_default(bus->sda);

    fd_i2cm_bus_info_t *bus_info = fd_i2cm_get_bus_info(bus);
    bus_info->enabled = false;
    NRF_TWIM_Type *twim = bus_info->twim;
    if (twim == 0) {
        return;
    }
 
    twim->ENABLE = TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos;

    twim->PSEL.SCL = 0xFFFFFFFF;
    twim->PSEL.SDA = 0xFFFFFFFF;
}

bool fd_i2cm_bus_is_enabled(const fd_i2cm_bus_t *bus) {
    fd_i2cm_bus_info_t *bus_info = fd_i2cm_get_bus_info(bus);
    NRF_TWIM_Type *twim = bus_info->twim;
    if (twim == 0) {
        return bus_info->enabled;
    }

    const uint32_t mask = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;
    return (twim->ENABLE & mask) == mask;
}

bool fd_i2cm_device_io(const fd_i2cm_device_t *device, const fd_i2cm_io_t *io) {
    fd_i2cm_bus_info_t *bus_info = fd_i2cm_get_bus_info(device->bus);
    NRF_TWIM_Type *twim = bus_info->twim;
    if (twim == 0) {
        return fd_i2cm_bitbang_device_io(device, io);
    }

    bool error = false;
    twim->ADDRESS = device->address;
    twim->EVENTS_STOPPED = 0;
    twim->EVENTS_ERROR = 0;
    twim->EVENTS_SUSPENDED = 0;
    twim->EVENTS_RXSTARTED = 0;
    twim->EVENTS_TXSTARTED = 0;
    twim->EVENTS_LASTRX = 0;
    twim->EVENTS_LASTTX = 0;
    twim->ERRORSRC = TWIM_ERRORSRC_ANACK_Msk | TWIM_ERRORSRC_DNACK_Msk | TWIM_ERRORSRC_OVERRUN_Msk;

    uint32_t timeout = device->bus->timeout;
    if (timeout == 0) {
        timeout = UINT32_MAX;
    }
    for (uint32_t i = 0; i < io->transfer_count; ++i) {
        bool last = i == (io->transfer_count - 1);
        twim->EVENTS_SUSPENDED = 0;
        const fd_i2cm_transfer_t *transfer = &io->transfers[i];
        if (transfer->direction == fd_i2cm_direction_tx) {
            twim->TXD.MAXCNT = transfer->byte_count;
            twim->TXD.PTR = (uint32_t)transfer->bytes;
            twim->SHORTS = last ? TWIM_SHORTS_LASTTX_STOP_Msk : TWIM_SHORTS_LASTTX_SUSPEND_Msk;
            twim->TASKS_STARTTX = 1;
        } else {
            twim->RXD.MAXCNT = transfer->byte_count;
            twim->RXD.PTR = (uint32_t)transfer->bytes;
#ifdef NRF52832_XXAA
            // !!! nRF52832 only has shortcut for "last tx suspend" and not "last rx suspend",
            // so that case is not supported on that MCU. An rx can only be the last transfer. -denis
            twim->SHORTS = TWIM_SHORTS_LASTRX_STOP_Msk;
#else // nRF52840
            twim->SHORTS = TWIM_SHORTS_LASTRX_STOP_Msk;
//            twim->SHORTS = last ? TWIM_SHORTS_LASTRX_STOP_Msk : TWIM_SHORTS_LASTRX_SUSPEND_Msk;
#endif
            twim->TASKS_STARTRX = 1;
        }
        if (i > 0) {
            twim->TASKS_RESUME = 1;
        }
        error = false;
        uint32_t count = 0;
        if (last) {
            while ((twim->EVENTS_STOPPED == 0) && (twim->EVENTS_ERROR == 0)) {
                if (++count >= timeout) {
                    error = true;
                    break;
                }
            }
        } else {
            while ((twim->EVENTS_SUSPENDED == 0) && (twim->EVENTS_ERROR == 0)) {
                if (++count >= timeout) {
                    error = true;
                    break;
                }
            }
        }
        if (twim->EVENTS_ERROR != 0) {
            break;
        }
        if (error) {
            break;
        }
    }

#ifndef FD_I2CM_DO_NOT_CLEAR_BUS_ON_ERROR
    if (error) {
        fd_i2cm_clear_bus(device->bus);
    }
#endif

    return (twim->EVENTS_ERROR == 0) && !error;
}

bool fd_i2cm_bus_wait(const fd_i2cm_bus_t *bus) {
    return true;
}