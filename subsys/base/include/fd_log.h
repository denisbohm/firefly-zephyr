#ifndef fd_log_h
#define fd_log_h

#include <stddef.h>
#include <stdint.h>

void fd_log_initialize(void);

size_t fd_log_get(uint8_t *data, size_t size);

#endif