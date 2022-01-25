#ifndef fd_boot_nrf53_h
#define fd_boot_nrf53_h

#include "fd_boot.h"

bool fd_boot_nrf53_flasher_initialize(void *context, uint32_t location, uint32_t length, fd_boot_error_t *error);

bool fd_boot_nrf53_flasher_write(void *context, uint32_t location, const uint8_t *data, uint32_t length, fd_boot_error_t *error);

bool fd_boot_nrf53_flasher_finalize(void *context, fd_boot_error_t *error);

bool fd_boot_nrf53_executor_start(fd_boot_update_interface_t *interface, fd_boot_error_t *error);

#endif