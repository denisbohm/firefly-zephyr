#ifndef fd_system_h
#define fd_system_h

#include <stdint.h>

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_system_version_t;

fd_system_version_t fd_system_get_version(void);

void fd_system_initialize(fd_system_version_t version);

#endif