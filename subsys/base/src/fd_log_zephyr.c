#include "fd_log.h"

#include <zephyr.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_backend_std.h>

typedef struct {
    uint32_t format;
    uint8_t __aligned(4) pipe_buffer[CONFIG_FIREFLY_SUBSYS_BASE_LOG_LIMIT];
    uint8_t __aligned(4) output_buffer[32];
    struct k_pipe pipe;
} fd_log_t;

fd_log_t fd_log;

size_t fd_log_get(uint8_t *data, size_t size) {
    size_t bytes_read = 0;
    k_pipe_get(&fd_log.pipe, data, size, &bytes_read, 0, K_NO_WAIT);
    return bytes_read;
}

int fd_log_write(uint8_t *data, size_t length, void *ctx) {
    size_t bytes_written = 0;
    k_pipe_put(&fd_log.pipe, data, length, &bytes_written, 0, K_NO_WAIT);
    return length;
}

LOG_OUTPUT_DEFINE(fd_log_output, fd_log_write, fd_log.output_buffer, sizeof(fd_log.output_buffer));

static void fd_log_init(const struct log_backend *const backend) {
    fd_log.format = LOG_OUTPUT_TEXT;

    k_pipe_init(&fd_log.pipe, fd_log.pipe_buffer, sizeof(fd_log.pipe_buffer));
}

static void fd_log_panic(struct log_backend const *const backend) {
    log_backend_deactivate(backend);
}

static void fd_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
    log_backend_std_dropped(&fd_log_output, cnt);
}

static void fd_log_process(const struct log_backend *const backend, union log_msg_generic *msg) {
    uint32_t flags = log_backend_std_get_flags();
    log_format_func_t log_output_func = log_format_func_t_get(fd_log.format);
    log_output_func(&fd_log_output, &msg->log, flags);
}

static int fd_log_format_set(const struct log_backend *const backend, uint32_t log_type) {
    fd_log.format = log_type;
    return 0;
}

static const struct log_backend_api fd_log_api = {
        .process = fd_log_process,
        .panic = fd_log_panic,
        .init = fd_log_init,
        .dropped = fd_log_dropped,
        .format_set = fd_log_format_set,
};

LOG_BACKEND_DEFINE(fd_log_backend, fd_log_api, true);
