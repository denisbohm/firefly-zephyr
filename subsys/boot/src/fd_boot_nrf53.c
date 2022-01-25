#include "fd_boot_nrf53.h"

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

bool fd_boot_nrf53_flash_erase(uint32_t address, uint32_t size) {
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

bool fd_boot_nrf53_flash_write(uint32_t address, const uint8_t *data, uint32_t size) {
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

bool fd_boot_nrf53_flasher_initialize(void *context, uint32_t location, uint32_t length, fd_boot_error_t *error) {
    if (!fd_boot_nrf53_flash_erase(location, length)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_nrf53_flasher_write(void *context, uint32_t location, const uint8_t *data, uint32_t length, fd_boot_error_t *error) {
    if (!fd_boot_nrf53_flash_write(location, data, length)) {
        fd_boot_set_error(error, 1);
        return false;
    }
    return true;
}

bool fd_boot_nrf53_flasher_finalize(void *context, fd_boot_error_t *error) {
    return true;
}

bool fd_boot_nrf53_executor_start(fd_boot_update_interface_t *interface, fd_boot_error_t *error) {
    fd_boot_info_executable_t info_executable;
    if (!interface->info.get_executable(&info_executable, error)) {
        return false;
    }
    SCB->VTOR = info_executable.location;
    uint32_t *vector_table = (uint32_t *)info_executable.location;
    uint32_t sp = vector_table[0];
    uint32_t pc = vector_table[1];
    __asm volatile(
        "   msr msp, %[sp]\n"
        "   msr psp, %[sp]\n"
        "   mov pc, %[pc]\n"
        :
        : [sp] "r" (sp), [pc] "r" (pc)
    );
    // should never reach here...
    fd_boot_set_error(error, 1);
    return false;
}
