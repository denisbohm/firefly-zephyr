#include "fd_assert_log.h"

#include "fd_assert.h"

#include <string.h>

#ifdef CONFIG_FIREFLY_SUBSYS_BASE_ASSERT_LOG_LIMIT
#define fd_assert_log_failure_limit CONFIG_FIREFLY_SUBSYS_BASE_ASSERT_LOG_LIMIT
#endif

#ifndef fd_assert_log_failure_limit
#define fd_assert_log_failure_limit 1
#endif

typedef struct {
    uint32_t failure_count;
    fd_assert_log_failure_t failures[fd_assert_log_failure_limit];
} fd_assert_log_t;

fd_assert_log_t fd_assert_log;

uint32_t fd_assert_log_get_failure_count(void) {
    return fd_assert_log.failure_count;
}

const fd_assert_log_failure_t *fd_assert_log_get_failure(uint32_t index) {
    if (index >= fd_assert_log.failure_count) {
        return 0;
    }
    return &fd_assert_log.failures[index];
}

void fd_assert_log_add_failure(const char *file, int line, const char *message) {
    if (fd_assert_log.failure_count >= fd_assert_log_failure_limit) {
        return;
    }

    const char *name = strrchr(file, '/');
    if (name != 0) {
        file = name + 1;
    }

    for (uint32_t i = 0; i < fd_assert_log.failure_count; ++i) {
        fd_assert_log_failure_t *failure = &fd_assert_log.failures[i];
        if ((failure->line == line) && (strncmp(failure->message, message, sizeof(failure->message)) == 0)) {
            ++failure->count;
            return;
        }
    }

    fd_assert_log_failure_t *failure = &fd_assert_log.failures[fd_assert_log.failure_count++];
    failure->file = file;
    failure->line = line;
    strncpy(failure->message, message, sizeof(failure->message));
    failure->message[sizeof(failure->message) - 1] = '\0';
    failure->count = 1;
}

void fd_assert_log_initialize(void) {
    memset(&fd_assert_log, 0, sizeof(fd_assert_log));

    fd_assert_set_callback(fd_assert_log_add_failure);
}