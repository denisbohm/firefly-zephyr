#include "fd_object_fifo.h"

#include "fd_assert.h"

void fd_object_fifo_initialize(fd_object_fifo_t *fifo, uint8_t *buffer, size_t size, size_t object_size) {
    memset(fifo, 0, sizeof(*fifo));
    fifo->buffer = buffer;
    fifo->object_size = object_size;
    fifo->size = size / object_size;
    fd_assert(fifo->size >= 1);
}

uint32_t fd_object_fifo_get_count(fd_object_fifo_t *fifo) {
    const uint32_t head = fifo->head;
    const uint32_t tail = fifo->tail;
    const uint32_t size = fifo->size;
    const uint32_t count = head <= tail ? (tail - head) : (size - head + tail + 1);
    return count;
}

bool fd_object_fifo_is_empty(fd_object_fifo_t *fifo) {
    return fifo->head == fifo->tail;
}

bool fd_object_fifo_is_full(fd_object_fifo_t *fifo) {
    return fd_object_fifo_get_count(fifo) >= (fifo->size - 1);
}

// Allocate the next object in the fifo.
// Return null if there is no space available.
// The object must be committed after writing the object content.
void *fd_object_fifo_allocate(fd_object_fifo_t *fifo) {
    if (fifo->is_allocated) {
        fd_assert_fail("missing commit");
        return 0;
    }
    if (fd_object_fifo_is_full(fifo)) {
        return 0;
    }

    fifo->is_allocated = true;
    return &fifo->buffer[fifo->tail * fifo->object_size];
}

uint32_t fd_object_fifo_get_next_tail(fd_object_fifo_t *fifo) {
    uint32_t tail = fifo->tail;
    uint32_t next_tail = tail + 1;
    if (next_tail >= fifo->size) {
        next_tail = 0;
    }
    return next_tail;
}

// Commit the currently allocated object into the fifo.
// This must be done after allocating the object and writing the object content.
// After being comitted the object can be viewed and deallocated.
void fd_object_fifo_commit(fd_object_fifo_t *fifo) {
    if (!fifo->is_allocated) {
        fd_assert_fail("missing allocate");
        return;
    }

    fifo->tail = fd_object_fifo_get_next_tail(fifo);
    fifo->is_allocated = false;
}

// View the nth committed object in the fifo.
// Return null if there isn't a committed object at that index.
void *fd_object_fifo_view(fd_object_fifo_t *fifo, uint32_t index) {
    uint32_t count = fd_object_fifo_get_count(fifo);
    if (index >= count) {
        return 0;
    }
    uint32_t position = (fifo->head + index) % fifo->size;
    return &fifo->buffer[position * fifo->object_size];
}

// Deallocate the first comitted object.
void fd_object_fifo_deallocate(fd_object_fifo_t *fifo) {
    if (fd_object_fifo_is_empty(fifo)) {
        fd_assert_fail("missing commit");
        return;
    }

    uint32_t head = fifo->head + 1;
    if (head >= fifo->size) {
        head = 0;
    }
    fifo->head = head;
}