#ifndef fd_storage_fatfs_h
#define fd_storage_fatfs_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void fd_storage_fatfs_initialize(void);

bool fd_storage_fatfs_open(void);

const char *fd_storage_fatfs_get_path(void);

// buffer must be at least 4096 bytes
bool fd_storage_fatfs_format(uint8_t *buffer, size_t size);

bool fd_storage_fatfs_mount(void);
void fd_storage_fatfs_unmount(void);

#endif