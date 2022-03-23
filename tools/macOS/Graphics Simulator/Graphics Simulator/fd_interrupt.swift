// uint32_t fd_interrupt_disable(void);
@_cdecl("fd_interrupt_disable")
func fd_interrupt_disable() -> UInt32 {
    return 0
}

// void fd_interrupt_enable(uint32_t state);
@_cdecl("fd_interrupt_enable")
func fd_interrupt_enable(state: UInt32) {
}

// void fd_interrupt_wait(void);
@_cdecl("fd_interrupt_wait")
func fd_interrupt_wait() {
}
