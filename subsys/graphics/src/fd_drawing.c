#include "fd_drawing.h"

#include "fd_assert.h"

void fd_drawing_plane_add(fd_drawing_plane_t *plane, size_t size, void *object, fd_drawing_class_t *class) {
    if (plane->drawing_count >= size) {
        fd_assert_fail("too many drawings");
        return;
    }
    plane->drawings[plane->drawing_count] = (fd_drawing_t) { .object = object, .class =  class };
    ++plane->drawing_count;
}
