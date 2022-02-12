#ifndef fd_version_h
#define fd_version_h

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_version_t;

bool fd_version_is_lt(const fd_version_t *a, const fd_version_t *b);

#endif