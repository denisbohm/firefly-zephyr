
// void fd_storage_fatfs_initialize(void);
@_cdecl("fd_storage_fatfs_initialize")
func fd_storage_fatfs_initialize() {
}

// bool fd_storage_fatfs_open(void);
@_cdecl("fd_storage_fatfs_open")
func fd_storage_fatfs_open() -> Bool {
    return true
}

// const char *fd_storage_fatfs_get_path(void);
@_cdecl("fd_storage_fatfs_get_path")
func fd_storage_fatfs_get_path() -> String {
    return "NAND:"
}

// buffer must be at least 4096 bytes
// bool fd_storage_fatfs_format(uint8_t *buffer, size_t size);
@_cdecl("fd_storage_fatfs_format")
func fd_storage_fatfs_format(buffer: UnsafePointer<UInt8>, size: size_t) -> Bool {
    return true
}

// uint64_t fd_storage_fatfs_get_free(void);
@_cdecl("fd_storage_fatfs_get_free")
func fd_storage_fatfs_get_free() -> UInt64 {
    return 0
}

// bool fd_storage_fatfs_mount(void);
@_cdecl("fd_storage_fatfs_mount")
func fd_storage_fatfs_mount() -> Bool {
    return true
}

// void fd_storage_fatfs_unmount(void);
@_cdecl("fd_storage_fatfs_unmount")
func fd_storage_fatfs_unmount() {
}
