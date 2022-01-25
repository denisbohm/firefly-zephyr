#include "fd_boot_flash.h"

#define NRF_APPLICATION

#ifdef NRF_APPLICATION
#include <nrf5340_application.h>
#define NRF_NVMC NRF_NVMC_S
#endif

#ifdef NRF_NETWORK
#include <nrf5340_network.h>
#define NRF_NVMC NRF_NVMC_NS
#endif

#define page_size 0x1000

bool fd_boot_flash_erase(uint32_t address, uint32_t size) {
    if ((address & (page_size - 1)) != 0) {
        return false;
    }
    // round up length to next page multiple
    size = (size + (page_size - 1)) & ~(page_size - 1);

    NRF_NVMC->CONFIG = 4; // PEEN
    uint32_t *erase_address = (uint32_t *)address;
    uint32_t erase_size = size;
    while (erase_size != 0) {
        while (!NRF_NVMC->READY) {
        }
        *erase_address = 0xffffffff;
        erase_address += page_size;
        erase_size -= page_size;
    }
    NRF_NVMC->CONFIG = 0;

    return true;
}

bool fd_boot_flash_write(uint32_t address, const uint8_t *data, uint32_t size) {
    if ((address & 0x3) != 0) {
        return false;
    }
    if ((size & 0x3) != 0) {
        return false;
    }
        
    NRF_NVMC->CONFIG = 1; // WEN
    uint32_t *write_address = (uint32_t *)address;
    uint32_t *write_data = (uint32_t *)data;
    uint32_t write_size = size;
    while (write_size != 0) {
        while (!NRF_NVMC->READY) {
        }
        *write_address++ = *write_data++;
        write_size -= sizeof(uint32_t);
    }
    NRF_NVMC->CONFIG = 0;

    return true;
}
