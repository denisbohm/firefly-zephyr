#ifndef fd_drawing_image_h
#define fd_drawing_image_h

#include "fd_drawing.h"
#include "fd_view_image.h"

typedef struct {
    fd_view_image_t view;
    
    fd_view_image_t previous_view;
    fd_graphics_area_t area;
    fd_graphics_point_t origin;
} fd_drawing_image_t;

extern fd_drawing_class_t fd_drawing_image_class;

#endif
