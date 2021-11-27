#include "fd_usb.h"

#include "fd_assert.h"

#include <usb/usb_device.h>

void fd_usb_initialize(void) {
    int err = usb_enable(NULL);
    fd_assert(err == 0);
}