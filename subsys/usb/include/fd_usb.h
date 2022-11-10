#ifndef fd_usb_h
#define fd_usb_h

#include <stdbool.h>

typedef struct {
    void (*connected)(void);
    void (*disconnected)(void);
} fd_usb_configuration_t;

void fd_usb_initialize(const fd_usb_configuration_t *configuration);

bool fd_usb_is_connected(void);

#endif
