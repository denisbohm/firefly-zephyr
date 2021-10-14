#ifndef fd_view_icon_h
#define fd_view_icon_h

#include "fd_view.h"

typedef struct {
    bool visible;
    fd_graphics_point_t location;
    fd_view_alignments_t alignments;
    fd_view_resource_bitmap_t bitmap;
    fd_graphics_color_t color;
} fd_view_icon_t;

#endif
