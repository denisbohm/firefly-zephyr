#ifndef fd_view_image_h
#define fd_view_image_h

#include "fd_view.h"

typedef struct {
    bool visible;
    fd_graphics_point_t location;
    fd_view_alignments_t alignments;
    fd_view_resource_image_t image;
} fd_view_image_t;

#endif
