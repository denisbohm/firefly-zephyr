#ifndef fd_assert_h
#define fd_assert_h

#include <stdint.h>

void fd_assert_initialize(void);

void fd_assert_failure(const char *file, int line, const char *message);

#define fd_assert(condition) if (!(condition)) fd_assert_failure(__FILE__, __LINE__, #condition)
#define fd_assert_fail(message) fd_assert_failure(__FILE__, __LINE__, message)

uint32_t fd_assert_get_count(void);

typedef void (*fd_assert_callback_t)(const char *file, int line, const char *message);

void fd_assert_set_callback(fd_assert_callback_t callback);

#endif
