#include "fd_system.h"

#include <string.h>

typedef struct {
    fd_system_version_t version;
} fd_system_t;

fd_system_t fd_system;

fd_system_version_t fd_system_get_version(void) {
    return fd_system.version;
}

void fd_system_initialize(fd_system_version_t version) {
    memset(&fd_system, 0, sizeof(fd_system));
    fd_system.version = version;
}
