#include "fd_assert.h"

static uint32_t fd_assert_count = 0;
static fd_assert_callback_t fd_assert_callback = 0;

void fd_assert_initialize(void) {
    fd_assert_count = 0;
    fd_assert_callback = 0;
}

void fd_assert_failure(const char *file, int line, const char *message) {
    ++fd_assert_count;

    if (fd_assert_callback) {
        fd_assert_callback(file, line, message);
    }
}

uint32_t fd_assert_get_count(void) {
    return fd_assert_count;
}

void fd_assert_set_callback(fd_assert_callback_t callback) {
    fd_assert_callback = callback;
}
