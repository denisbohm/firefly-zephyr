#ifndef fd_boot_error_h
#define fd_boot_error_h

#include <stdint.h>

typedef struct {
    uint32_t code;
} fd_boot_error_t;

void fd_boot_set_error(fd_boot_error_t *error, uint32_t code);

#endif
