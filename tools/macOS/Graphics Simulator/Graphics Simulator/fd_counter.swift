import Foundation

class SimulateCounter {
    
    var tasks: [fd_counter_task_t]
    
    init() {
        self.tasks = []
    }
    
    func fd_counter_task_start(task: UnsafePointer<fd_counter_task_t>) {
        var counter_task = fd_counter_task_t()
        counter_task.function = task.pointee.function;
        counter_task.context = task.pointee.context;
        counter_task.interval = task.pointee.interval;
        self.tasks.append(counter_task)
    }

}

var simulateCounter = SimulateCounter()

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
func fd_counter_task_start(task: UnsafePointer<fd_counter_task_t>) {
    simulateCounter.fd_counter_task_start(task: task)
}
