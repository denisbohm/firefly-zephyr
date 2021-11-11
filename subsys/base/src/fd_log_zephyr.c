#include "fd_log.h"

#include <logging/log_backend.h>
#include <logging/log_backend_std.h>
#include <logging/log_output.h>

typedef struct {
    uint8_t output_buffer[32];
    uint8_t __aligned(4) pipe_buffer[100];
    struct k_pipe pipe;
} fd_log_t;

fd_log_t fd_log;

int fd_log_output_string(uint8_t *data, size_t length, void *ctx) {
    size_t bytes_written = 0;
    k_pipe_put(&fd_log.pipe, data, length, &bytes_written, 0, K_NO_WAIT);
    return length;
}

LOG_OUTPUT_DEFINE(fd_log_output, fd_log_output_string, fd_log.output_buffer, sizeof(fd_log.output_buffer));

void fd_log_process(const struct log_backend *const backend, union log_msg2_generic *msg) {
    log_output_msg2_process(&fd_log_output, &msg->log, 0);
}

void fd_log_put(const struct log_backend *const backend, struct log_msg *msg) {
    log_backend_std_put(&fd_log_output, 0, msg);
}

void fd_log_put_sync_string(
    const struct log_backend *const backend,
    struct log_msg_ids src_level,
    uint32_t timestamp,
    const char *fmt,
    va_list ap
) {
}

void fd_log_put_sync_hexdump(
    const struct log_backend *const backend,
    struct log_msg_ids src_level,
    uint32_t timestamp,
    const char *metadata,
    const uint8_t *data,
    uint32_t len
) {
}

void fd_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
    log_backend_std_dropped(&fd_log_output, cnt);
}

void fd_log_panic(const struct log_backend *const backend) {
    log_backend_std_panic(&fd_log_output);
}

void fd_log_init(const struct log_backend *const backend) {
    memset(&fd_log, 0, sizeof(fd_log));

    k_pipe_init(&fd_log.pipe, fd_log.pipe_buffer, sizeof(fd_log.pipe_buffer));
}

const struct log_backend_api fd_log_backend_api = {
    .process = fd_log_process,
    .put = fd_log_put,
    .put_sync_string = 0, // fd_log_put_sync_string,
    .put_sync_hexdump = 0, // fd_log_put_sync_hexdump,
    .dropped = fd_log_dropped,
    .panic = fd_log_panic,
    .init = fd_log_init,
};

LOG_BACKEND_DEFINE(fd_log_backend, fd_log_backend_api, true);

void fd_log_initialize(void) {
    // log_backend_activate(&fd_log_backend, 0);
}

size_t fd_log_get(uint8_t *data, size_t size) {
    size_t bytes_read = 0;
    k_pipe_get(&fd_log.pipe, data, size, &bytes_read, 0, K_NO_WAIT);
    return bytes_read;
}