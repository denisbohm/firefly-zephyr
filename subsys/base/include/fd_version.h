#ifndef fd_version_h
#define fd_version_h

#include <stdint.h>

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_version_t;

#endif