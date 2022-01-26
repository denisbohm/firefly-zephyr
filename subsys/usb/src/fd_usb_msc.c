#include "fd_usb_msc.h"

#include <string.h>

typedef struct {
} fd_usb_msc_t;

fd_usb_msc_t fd_usb_msc;

void fd_usb_msc_initialize(void) {
    memset(&fd_usb_msc, 0, sizeof(fd_usb_msc));
}