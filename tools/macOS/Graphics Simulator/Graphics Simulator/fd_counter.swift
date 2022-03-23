import Foundation

// void fd_counter_initialize(void);
@_cdecl("fd_counter_initialize")
func fd_counter_initialize() {
}

// double fd_counter_get_uptime(void);
@_cdecl("fd_counter_get_uptime")
func fd_counter_get_uptime() -> Double {
    return Double(Date().timeIntervalSince1970)
}

// void fd_counter_task_start(fd_counter_task_t *task);
@_cdecl("fd_counter_task_start")
func fd_counter_task_start(task: fd_counter_task_t) {
}
