#ifndef fd_fifo_h
#define fd_fifo_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} fd_fifo_t;

void fd_fifo_initialize(fd_fifo_t *fifo, uint8_t *data, size_t size);
void fd_fifo_flush(fd_fifo_t *fifo);
bool fd_fifo_is_empty(fd_fifo_t *fifo);
uint32_t fd_fifo_get_count(fd_fifo_t *fifo);
bool fd_fifo_get(fd_fifo_t *fifo, uint8_t *byte);
bool fd_fifo_put(fd_fifo_t *fifo, uint8_t data);

#endif
