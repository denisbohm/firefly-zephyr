#ifndef fd_boot_range_h
#define fd_boot_range_h

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t location;
    uint32_t length;
} fd_boot_range_t;

#endif
