#include "fd_storage_fatfs.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/flash_map.h>
#include <ff.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>

#ifdef FD_STORAGE_FATFS_TEST_QSPI
#include <nrf.h>
#include <nrfx_qspi.h>
#endif

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

#ifdef FD_STORAGE_FATFS_TEST_QSPI

static void fd_storage_fatfs_qspi_instruction(uint8_t opcode, uint8_t *tx, uint8_t *rx) {
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode = opcode,
        .length = NRF_QSPI_CINSTR_LEN_2B,
        .io2_level = false,
        .io3_level = false,
        .wipwait = true,
        .wren = true,
    };
    int err = nrfx_qspi_cinstr_xfer(&cinstr_cfg, tx, rx);
    fd_assert(err == NRFX_SUCCESS);
}

void fd_storage_fatfs_quad(void) {
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode = 0xB7,
        .length = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = false,
        .io3_level = false,
        .wipwait = true,
        .wren = true,
    };
    int err = nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, 0);
    fd_assert(err == NRFX_SUCCESS);
}

void fd_storage_fatfs_wait(void) {
    while (true) {
        uint8_t status;
        nrf_qspi_cinstr_conf_t cinstr_cfg = {
            .opcode = 0x05,
            .length = NRF_QSPI_CINSTR_LEN_2B,
            .io2_level = false,
            .io3_level = false,
            .wipwait = true,
            .wren = true,
        };
        int err = nrfx_qspi_cinstr_xfer(&cinstr_cfg, 0, &status);
        if (err != NRFX_SUCCESS) {
            continue;
        }
        if ((status & 0b1) == 0) {
            break;
        }
    }
}

void fd_storage_fatfs_test_write(uint32_t address, uint8_t *tx_data) {
    int err = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, address);
    fd_assert(err == NRFX_SUCCESS);
    fd_storage_fatfs_wait();
    uint8_t rx_data[4];
    err = nrfx_qspi_read(rx_data, sizeof(rx_data), address);
    fd_assert(err == NRFX_SUCCESS);
    fd_storage_fatfs_wait();
    const uint8_t erased[] = {0xff, 0xff, 0xff, 0xff};
    fd_assert(memcmp(rx_data, erased, sizeof(rx_data)) == 0);
    err = nrfx_qspi_write(tx_data, 4, address);
    fd_assert(err == NRFX_SUCCESS);
    fd_storage_fatfs_wait();
    err = nrfx_qspi_read(rx_data, sizeof(rx_data), address);
    fd_assert(err == NRFX_SUCCESS);
    fd_storage_fatfs_wait();
    fd_assert(memcmp(rx_data, tx_data, sizeof(rx_data)) == 0);
}

void fd_storage_fatfs_test_read(uint32_t address, uint8_t *rx_data) {
    int err = nrfx_qspi_read(rx_data, 4, address);
    fd_assert(err == NRFX_SUCCESS);
    fd_storage_fatfs_wait();
}

void fd_storage_fatfs_test(void) {
    fd_storage_fatfs_quad();

    uint8_t rx[1];
    fd_storage_fatfs_qspi_instruction(0x05, 0, rx);
    uint8_t tx[] = { 0b00000000 };
    fd_storage_fatfs_qspi_instruction(0x01, tx, 0);
    fd_storage_fatfs_qspi_instruction(0x05, 0, rx);

    uint8_t tx_0[] = {0, 1, 2, 3};
    uint8_t tx_1[] = {4, 5, 6, 7};
    uint8_t tx_2[] = {8, 9, 10, 11};
    uint8_t tx_3[] = {12, 13, 14, 15};
    uint8_t tx_4[] = {16, 17, 18, 19};
    fd_storage_fatfs_test_write(0x0000000, tx_0);
    fd_storage_fatfs_test_write(0x1000000, tx_1);
    fd_storage_fatfs_test_write(0x4000000 - 0x0800000 - 0x0010000, tx_2);
    fd_storage_fatfs_test_write(0x4000000 - 0x0800000, tx_3);
    fd_storage_fatfs_test_write(0x4000000 - 0x0010000, tx_4);
    uint8_t rx_0[4];
    uint8_t rx_1[4];
    uint8_t rx_2[4];
    uint8_t rx_3[4];
    uint8_t rx_4[4];
    uint8_t rx_5[4];
    fd_storage_fatfs_test_read(0x0000000, rx_0);
    fd_storage_fatfs_test_read(0x1000000, rx_1);
    fd_storage_fatfs_test_read(0x4000000 - 0x0800000 - 0x0010000, rx_2);
    fd_storage_fatfs_test_read(0x4000000 - 0x0800000, rx_3);
    fd_storage_fatfs_test_read(0x4000000 - 0x0010000, rx_4);
    fd_storage_fatfs_test_read(0x4000000, rx_5);
}
#endif

bool fd_storage_fatfs_open(void) {
    fd_storage_fatfs.fs_mount.storage_dev = (void *)FLASH_AREA_ID(storage);
    unsigned int id = (uintptr_t)fd_storage_fatfs.fs_mount.storage_dev;
    const struct flash_area *flash_area;
    int rc = flash_area_open(id, &flash_area);
    if (rc != 0) {
        return false;
    }

#ifdef FD_STORAGE_FATFS_TEST_QSPI
    fd_storage_fatfs_test();
#endif

#ifdef FD_STORAGE_FATFS_TEST_SPIM
    rc = flash_area_erase(flash_area, 0, 65536);
    fd_assert(rc == 0);
    uint8_t read[4];
    rc = flash_area_read(flash_area, 0, read, sizeof(read));
    fd_assert(rc == 0);
    uint8_t write[4] = { 5, 1, 3, 9 };
    rc = flash_area_write(flash_area, 0, write, sizeof(write));
    fd_assert(rc == 0);
    rc = flash_area_read(flash_area, 0, read, sizeof(read));
    fd_assert(rc == 0);
    fd_assert(memcmp(read, write, sizeof(write)) == 0);
#endif

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