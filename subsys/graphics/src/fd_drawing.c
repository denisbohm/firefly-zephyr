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

fd_graphics_point_t fd_drawing_align(fd_graphics_area_t area, fd_graphics_point_t location, fd_view_alignments_t alignments) {
    fd_graphics_point_t origin = { .x = 0, .y = 0 };
    switch (alignments.x) {
        case fd_view_alignment_origin:
            origin.x = location.x;
            break;
        case fd_view_alignment_min:
            origin.x = location.x - area.x;
            break;
        case fd_view_alignment_max:
            origin.x = location.x - (area.x + area.width);
            break;
        case fd_view_alignment_center:
            origin.x = location.x - (area.x + area.width / 2);
            break;
    }
    switch (alignments.y) {
        case fd_view_alignment_origin:
            origin.y = location.y;
            break;
        case fd_view_alignment_min:
            origin.y = location.y - area.y;
            break;
        case fd_view_alignment_max:
            origin.y = location.y - (area.y + area.height);
            break;
        case fd_view_alignment_center:
            origin.y = location.y - (area.y + area.height / 2);
            break;
    }
    return origin;
}
