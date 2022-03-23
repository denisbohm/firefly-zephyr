#include "fd_storage_fatfs.h"

#include "fd_assert.h"

#include <zephyr.h>
#include <usb/usb_device.h>
#include <fs/fs.h>
#include <storage/flash_map.h>
#include <ff.h>
#include <drivers/flash.h>
#include <device.h>

#include <string.h>

typedef struct {
    const char *path;
    struct fs_mount_t fs_mount;
    FATFS fatfs;
    bool is_open;
    bool is_mounted;
} fd_storage_fatfs_t;

fd_storage_fatfs_t fd_storage_fatfs;

void fd_storage_fatfs_initialize(void) {
    memset(&fd_storage_fatfs, 0, sizeof(fd_storage_fatfs));
}

uint64_t fd_storage_fatfs_get_free(void) {
    FATFS *fs;
    DWORD free_clusters;
    FRESULT result = f_getfree("NAND:", &free_clusters, &fs);
    if (result != FR_OK) {
        return 0.0f;
    }
    // DWORD total_sectors = (fs->n_fatent - 2) * fs->csize;
    DWORD free_sectors = free_clusters * fs->csize;
    return free_sectors * 512;
}

#ifdef fd_storage_fatfs_32bit_mode
static void fd_storage_fatfs_qspi_instruction(uint8_t opcode) {
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode = opcode,
        .length = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = false,
        .io3_level = false,
        .wipwait = true,
        .wren = true,
    };
    int err = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    fd_assert(err == NRFX_SUCCESS);
}
#endif

bool fd_storage_fatfs_open(void) {
#ifdef fd_storage_fatfs_32bit_mode
    // This should really be in the qspi driver, but there isn't any way to do that currently... -denis
    fd_storage_fatfs_qspi_instruction(0xB7); // ENTER 4-BYTE ADDRESS MODE
#endif

    fd_storage_fatfs.fs_mount.storage_dev = (void *)FLASH_AREA_ID(storage);
    unsigned int id = (uintptr_t)fd_storage_fatfs.fs_mount.storage_dev;
    const struct flash_area *flash_area;
    int rc = flash_area_open(id, &flash_area);
    if (rc != 0) {
        return false;
    }
    fd_storage_fatfs.is_open = true;
    return true;
}

const char *fd_storage_fatfs_get_path(void) {
    return "NAND:";
}

bool fd_storage_fatfs_mount(void) {
    fd_assert(fd_storage_fatfs.is_open);
    fd_storage_fatfs.fs_mount.type = FS_FATFS;
    fd_storage_fatfs.fs_mount.fs_data = &fd_storage_fatfs.fatfs;
    fd_storage_fatfs.fs_mount.mnt_point = "/NAND:";
    int rc = fs_mount(&fd_storage_fatfs.fs_mount);
    if (rc != 0) {
        return false;
    }
    fd_storage_fatfs.is_mounted = true;
    return true;
}

void fd_storage_fatfs_unmount(void) {
    fd_assert(fd_storage_fatfs.is_open);
    int rc = fs_unmount(&fd_storage_fatfs.fs_mount);
    fd_assert(rc == 0);
    fd_storage_fatfs.is_mounted = false;
}

bool fd_storage_fatfs_format(uint8_t *buffer, size_t size) {
    if (size < 4096) {
        return false;
    }
    fd_assert(fd_storage_fatfs.is_open);
    bool is_mounted = fd_storage_fatfs.is_mounted;
    if (is_mounted) {
        fd_storage_fatfs_unmount();
    }
    FRESULT result = f_mkfs("NAND:", 0, buffer, size);
    if (result != FR_OK) {
        return false;
    }
    if (is_mounted) {
        return fd_storage_fatfs_mount();
    }
    return true;
}