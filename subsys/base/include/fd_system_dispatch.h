#ifndef fd_system_dispatch_h
#define fd_system_dispatch_h

#include "fd_system.h"

void fd_system_dispatch_initialize(void);

#ifndef fd_system_dispatch_failure_message_limit
#define fd_system_dispatch_failure_message_limit 32
#endif

typedef struct {
    const char *file;
    int line;
    char message[fd_system_dispatch_failure_message_limit];
} fd_system_dispatch_failure_t;

uint32_t fd_system_dispatch_get_failure_count(void);
const fd_system_dispatch_failure_t *fd_system_dispatch_get_failure(uint32_t index);

#endif