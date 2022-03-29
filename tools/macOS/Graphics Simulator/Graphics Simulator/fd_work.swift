
//void fd_work_initialize(void);
@_cdecl("fd_work_initialize")
func fd_work_initialize() {
}

//void fd_work_queue_initialize(fd_work_queue_t *queue, const fd_work_queue_configuration_t *configuration);
@_cdecl("fd_work_queue_initialize")
func fd_work_queue_initialize(queue: UnsafePointer<fd_work_queue_t>, configuration: UnsafePointer<fd_work_queue_configuration_t>) {
}

//void fd_work_queue_clear(fd_work_queue_t *queue);
@_cdecl("fd_work_queue_clear")
func fd_work_queue_clear(queue: UnsafePointer<fd_work_queue_t>) {
}

//fd_work_queue_submit_result_t fd_work_queue_submit(fd_work_queue_t *queue, fd_work_task_t task);
@_cdecl("fd_work_queue_submit")
func fd_work_queue_submit(queue: UnsafePointer<fd_work_queue_t>, task: fd_work_task_t) -> fd_work_queue_submit_result_t {
    return fd_work_queue_submit_result_queued;
}

//fd_work_queue_submit_result_t fd_work_queue_submit_with_delay(fd_work_queue_t *queue, fd_work_task_t task, float delay);
@_cdecl("fd_work_queue_submit_with_delay")
func fd_work_queue_submit_with_delay(queue: UnsafePointer<fd_work_queue_t>, task: fd_work_task_t, delay: Float) -> fd_work_queue_submit_result_t {
    return fd_work_queue_submit_result_queued;
}

//fd_work_queue_wait_result_t fd_work_queue_cancel_task(fd_work_queue_t *queue, fd_work_task_t task);
@_cdecl("fd_work_queue_cancel_task")
func fd_work_queue_cancel_task(queue: UnsafePointer<fd_work_queue_t>, task: fd_work_task_t) -> fd_work_queue_wait_result_t {
    return fd_work_queue_wait_result_no_wait;
}

//fd_work_queue_wait_result_t fd_work_queue_wait_for_task(fd_work_queue_t *queue, fd_work_task_t task);
@_cdecl("fd_work_queue_wait_for_task")
func fd_work_queue_wait_for_task(queue: UnsafePointer<fd_work_queue_t>, task: fd_work_task_t) -> fd_work_queue_wait_result_t {
    return fd_work_queue_wait_result_no_wait;
}

//fd_work_queue_wait_result_t fd_work_queue_wait(fd_work_queue_t *queue);
@_cdecl("fd_work_queue_wait")
func fd_work_queue_wait(queue: UnsafePointer<fd_work_queue_t>) -> fd_work_queue_wait_result_t {
    return fd_work_queue_wait_result_no_wait;
}

//bool fd_work_queue_process(fd_work_queue_t *queue, float elapsed);
@_cdecl("fd_work_queue_process")
func fd_work_queue_process(queue: UnsafePointer<fd_work_queue_t>, elapsed: Float) -> Bool {
    return false;
}
