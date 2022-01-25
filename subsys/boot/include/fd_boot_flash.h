#ifndef fd_boot_flash_h
#define fd_boot_flash_h

#include <stdbool.h>
#include <stdint.h>

bool fd_boot_flash_erase(uint32_t address, uint32_t size);

bool fd_boot_flash_write(uint32_t address, const uint8_t *data, uint32_t size);

#endif