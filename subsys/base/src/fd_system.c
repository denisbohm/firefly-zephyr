#include "fd_system.h"

#include <string.h>

typedef struct {
    const fd_system_identity_t *identity;
} fd_system_t;

fd_system_t fd_system;

const fd_system_identity_t *fd_system_get_version(void) {
    return fd_system.identity;
}

void fd_system_initialize(const fd_system_identity_t *identity) {
    memset(&fd_system, 0, sizeof(fd_system));
    fd_system.identity = identity;
}
