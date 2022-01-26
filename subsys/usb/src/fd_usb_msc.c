#include <zephyr.h>
#include <usb/usb_device.h>
#include <fs/fs.h>
#include <storage/flash_map.h>
#include <ff.h>
#include <drivers/flash.h>
#include <device.h>

#include "fd_assert.h"

#include <string.h>

typedef struct {
} fd_usb_msc_t;

fd_usb_msc_t fd_usb_msc;

void fd_usb_msc_initialize(void) {
    memset(&fd_usb_msc, 0, sizeof(fd_usb_msc));
}