#ifndef fd_system_h
#define fd_system_h

#include "fd_version.h"

#include <stdint.h>

typedef struct {
    fd_version_t version;
    const char *identifier;
} fd_system_identity_t;

const fd_system_identity_t *fd_system_get_identity(void);

void fd_system_initialize(const fd_system_identity_t *identity);

#endif