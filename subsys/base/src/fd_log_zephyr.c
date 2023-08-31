#include "fd_log.h"

#include "fd_fifo.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>

typedef struct {
    struct k_sem semaphore;
    fd_fifo_t fifo;
    uint32_t format;
    uint8_t output_buffer[32];
    uint8_t buffer[CONFIG_FIREFLY_SUBSYS_BASE_LOG_LIMIT];
} fd_log_t;

fd_log_t fd_log;

size_t fd_log_get(uint8_t *data, size_t size) {
    k_sem_take(&fd_log.semaphore, K_FOREVER);
    uint32_t length = 0;
    uint8_t byte;
    while ((length < size) && fd_fifo_get(&fd_log.fifo, &byte)) {
        *data++ = byte;
        ++length;
    }
    k_sem_give(&fd_log.semaphore);
    return length;
}

int fd_log_write(uint8_t *data, size_t length, void *ctx) {
    k_sem_take(&fd_log.semaphore, K_FOREVER);

    // make room
    uint32_t available = fd_log.fifo.size - fd_fifo_get_count(&fd_log.fifo);
    for (uint32_t i = available; i <= length; ++i) {
        uint8_t byte;
        if (!fd_fifo_get(&fd_log.fifo, &byte)) {
            break;
        }
    }

    // write data
    for (uint32_t i = 0; i < length; ++i) {
        fd_fifo_put(&fd_log.fifo, data[i]);
    }

    k_sem_give(&fd_log.semaphore);
    return length;
}

LOG_OUTPUT_DEFINE(fd_log_output, fd_log_write, fd_log.output_buffer, sizeof(fd_log.output_buffer));

static void fd_log_init(const struct log_backend *const backend) {
    fd_log.format = LOG_OUTPUT_TEXT;

    fd_fifo_initialize(&fd_log.fifo, fd_log.buffer, sizeof(fd_log.buffer));

    k_sem_init(&fd_log.semaphore, 1, 1);
}

static void fd_log_panic(struct log_backend const *const backend) {
}

static void fd_log_dropped(const struct log_backend *const backend, uint32_t cnt) {
}

static void fd_log_process(const struct log_backend *const backend, union log_msg_generic *msg) {
    const uint32_t flags = LOG_OUTPUT_FLAG_CRLF_LFONLY | LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
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
