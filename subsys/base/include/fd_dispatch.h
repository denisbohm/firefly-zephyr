#ifndef fd_dispatch_h
#define fd_dispatch_h

#include "fd_binary.h"
#include "fd_envelope.h"

typedef bool (*fd_dispatch_respond_t)(fd_binary_t *message, fd_envelope_t *envelope);

typedef bool (*fd_dispatch_process_t)(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond);

typedef bool (*fd_dispatch_filter_t)(const fd_envelope_t *envelope);

void fd_dispatch_initialize(fd_dispatch_respond_t respond);

void fd_dispatch_add_process(fd_dispatch_process_t process, fd_dispatch_filter_t filter);

bool fd_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope);

#endif