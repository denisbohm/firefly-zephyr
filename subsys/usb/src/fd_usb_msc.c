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
    struct fs_mount_t fs_mount;
    FATFS fatfs;
} fd_usb_msc_t;

fd_usb_msc_t fd_usb_msc;

void fd_usb_msc_initialize(void) {
    memset(&fd_usb_msc, 0, sizeof(fd_usb_msc));

    fd_usb_msc.fs_mount.storage_dev = (void *)FLASH_AREA_ID(storage);
    unsigned int id = (uintptr_t)fd_usb_msc.fs_mount.storage_dev;
    const struct flash_area *flash_area;
    int rc = flash_area_open(id, &flash_area);
    fd_assert(rc == 0);
    
    fd_usb_msc.fs_mount.type = FS_FATFS;
    fd_usb_msc.fs_mount.fs_data = &fd_usb_msc.fatfs;
    fd_usb_msc.fs_mount.mnt_point = "/NAND:";
    rc = fs_mount(&fd_usb_msc.fs_mount);
    fd_assert(rc == 0);
}