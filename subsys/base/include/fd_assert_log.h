#ifndef fd_assert_log_h
#define fd_assert_log_h

#include "fd_system.h"

void fd_assert_log_initialize(void);

#ifndef fd_assert_log_failure_message_limit
#define fd_assert_log_failure_message_limit 32
#endif

typedef struct {
    const char *file;
    int line;
    char message[fd_assert_log_failure_message_limit];
    uint32_t count;
} fd_assert_log_failure_t;

uint32_t fd_assert_log_get_failure_count(void);
const fd_assert_log_failure_t *fd_assert_log_get_failure(uint32_t index);

void fd_assert_log_add_failure(const char *file, int line, const char *message);

#endif