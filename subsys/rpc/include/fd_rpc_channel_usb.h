#ifndef fd_rpc_channel_usb_h
#define fd_rpc_channel_usb_h

#include "fd_source.h"

#include <zephyr/kernel.h>

fd_source_push()

// unfortunately, the USB serial number is accessed before usb_enable is called, so need to set it up before main -denis
void fd_rpc_channel_usb_set_serial_number(const char *serial_number);
//
#define fd_rpc_channel_usb_initialize_serial_number(serial_number)\
    int fd_rpc_channel_usb_sys_init(void) { fd_rpc_channel_usb_set_serial_number(serial_number); return 0; }\
    SYS_INIT(fd_rpc_channel_usb_sys_init, POST_KERNEL, 60 // must be > CONFIG_FLASH_INIT_PRIORITY -denis */);

typedef struct {
    struct k_work_q *work_queue;
} fd_rpc_channel_usb_configuration_t;

void fd_rpc_channel_usb_initialize(const fd_rpc_channel_usb_configuration_t *configuration);

typedef struct {
    void (*connected)(void);
    void (*disconnected)(void);
} fd_rpc_channel_usb_consumer_t;

void fd_rpc_channel_usb_set_consumer(const fd_rpc_channel_usb_consumer_t *consumer);

fd_source_pop()

#endif