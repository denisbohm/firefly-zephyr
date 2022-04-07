#ifndef fd_test_h
#define fd_test_h

#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_FIREFLY_SUBSYS_BASE_TEST

void fd_test_initialize(void);
void fd_test_add(const char *module, int identifier, bool (*function)(void *context), void *context);
void fd_test_add_on_count(const char *module, int identifier, uint32_t count);

bool fd_test_should_fault(const char *module, int identifier);
#define fd_test_should_fault_in_function() fd_test_should_fault(__func__, 0)

bool fd_test_should_fault(const char *module, int identifier);
#define fd_test_should_fault_in_function() fd_test_should_fault(__func__, 0)

#else

#define fd_test_should_fault(...) false
#define fd_test_should_fault_in_function() false

#endif

#endif