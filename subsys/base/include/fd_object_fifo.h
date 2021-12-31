#ifndef fd_object_fifo_h
#define fd_object_fifo_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// An object fifo contains a number of objects all of the same preconfigured size.
// To put an object into the fifo first allocate the object, then write the object content, then commit the object.
// To get an object from the fifo first view the object, read the object content, then deallocate the object.
//
// It is safe to put objects (allocate & commit) from one ISR/thread and get objects (view & deallocate) from one different ISR/thread.
//
// The intention of this class is to:
// a) Be safe for passing objects from one thread to another.
// b) Minimize buffers by allocating the object and filling its content before committing.
// c) Support robust communications by allowing objects to be viewed for sending and later deallocated on acknowledgement of reception.

typedef struct {
    uint8_t *buffer;
    size_t object_size;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
    bool is_allocated;
} fd_object_fifo_t;

// Note that due to the implementation, the fifo will hold one less object than size / object_size...
void fd_object_fifo_initialize(fd_object_fifo_t *fifo, uint8_t *buffer, size_t size, size_t object_size);

// Allocate the next object in the fifo.
// Return null if there is no space available.
// The object must be committed after writing the object content.
void *fd_object_fifo_allocate(fd_object_fifo_t *fifo);

// Commit the currently allocated object into the fifo.
// This must be done after allocating the object and writing the object content.
// After being comitted the object can be viewed and deallocated.
void fd_object_fifo_commit(fd_object_fifo_t *fifo);

// View the nth committed object in the fifo.
// Return null if there isn't a committed object at that index.
void *fd_object_fifo_view(fd_object_fifo_t *fifo, uint32_t index);

// Deallocate the first comitted object.
void fd_object_fifo_deallocate(fd_object_fifo_t *fifo);

#endif