#ifndef fd_boot_version_h
#define fd_boot_version_h

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_boot_version_t;

#endif
