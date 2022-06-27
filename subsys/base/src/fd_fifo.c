#include "fd_fifo.h"

void fd_fifo_initialize(fd_fifo_t *fifo, uint8_t *data, size_t size) {
    fifo->buffer = data;
    fifo->size = size;
    fifo->head = 0;
    fifo->tail = 0;
}

void fd_fifo_flush(fd_fifo_t *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
}

bool fd_fifo_is_empty(fd_fifo_t *fifo) {
    return fifo->head == fifo->tail;
}

bool fd_fifo_get(fd_fifo_t *fifo, uint8_t *byte) {
    bool valid = fifo->head != fifo->tail;
    if (valid) {
        *byte = fifo->buffer[fifo->head];
        size_t head = fifo->head + 1;
        if (head >= fifo->size) {
            head = 0;
        }
        fifo->head = head;
    }
    return valid;
}

bool fd_fifo_put(fd_fifo_t *fifo, uint8_t data) {
    size_t tail = fifo->tail;
    size_t next_tail = tail + 1;
    if (next_tail >= fifo->size) {
        next_tail = 0;
    }
    bool valid = next_tail != fifo->head;
    if (valid) {
        fifo->buffer[tail] = data;
        fifo->tail = next_tail;
    } else {
        static int count = 0;
        ++count;
    }
    return valid;
}
