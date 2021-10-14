#ifndef fd_view_rectangle_h
#define fd_view_rectangle_h

#include "fd_view.h"

typedef struct {
    bool visible;
    fd_graphics_point_t location;
    fd_view_alignments_t alignments;
    fd_graphics_area_t area;
    fd_graphics_color_t color;
} fd_view_rectangle_t;

#endif
