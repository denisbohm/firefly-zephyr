#ifndef fd_rpc_channel_thread_h
#define fd_rpc_channel_thread_h

#include "fd_source.h"

#include <openthread/thread.h>

#include <zephyr/kernel.h>

fd_source_push()

typedef struct {
    const char *name;
    struct k_work_q *work_queue;
    uint16_t service_port;
} fd_rpc_channel_thread_configuration_t;

void fd_rpc_channel_thread_initialize(const fd_rpc_channel_thread_configuration_t *configuration);

otInstance *fd_rpc_channel_thread_get_ot_instance(void);

typedef enum {
    fd_rpc_channel_thread_state_none,
    fd_rpc_channel_thread_state_started,
    fd_rpc_channel_thread_state_listening,
    fd_rpc_channel_thread_state_connected,
} fd_rpc_channel_thread_state_t;

typedef struct {
    fd_rpc_channel_thread_state_t state;
    otDeviceRole role;
} fd_rpc_channel_thread_status_t;

typedef struct {
    void (*status_changed)(fd_rpc_channel_thread_status_t status);
    void (*connected)(void);
    void (*disconnected)(void);
} fd_rpc_channel_thread_consumer_t;

void fd_rpc_channel_thread_set_consumer(const fd_rpc_channel_thread_consumer_t *consumer);

fd_rpc_channel_thread_status_t fd_rpc_channel_thread_get_status(void);

void fd_rpc_channel_thread_set_dataset(otOperationalDataset dataset);

fd_source_pop()

#endif