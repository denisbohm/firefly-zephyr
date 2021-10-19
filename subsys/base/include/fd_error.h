#ifndef fd_error_h
#define fd_error_h

#include <stdint.h>

typedef struct {
    uint32_t status;
    char message[64];
} fd_error_t;

#endif
