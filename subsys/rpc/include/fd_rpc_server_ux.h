#ifndef fd_rpc_server_ux_h
#define fd_rpc_server_ux_h

#include "fd_source.h"

#include <zephyr/kernel.h>

fd_source_push()

typedef struct {
    struct k_work_q *work_queue;
} fd_rpc_server_ux_configuration_t;

void fd_rpc_server_ux_initialize(const fd_rpc_server_ux_configuration_t *configuration);

fd_source_pop()

#endif