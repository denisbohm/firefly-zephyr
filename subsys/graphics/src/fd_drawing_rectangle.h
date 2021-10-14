#ifndef fd_drawing_rectangle_h
#define fd_drawing_rectangle_h

#include "fd_drawing.h"
#include "fd_view_rectangle.h"

typedef struct {
    fd_view_rectangle_t view;
    
    fd_view_rectangle_t previous_view;
    fd_graphics_area_t area;
} fd_drawing_rectangle_t;

extern fd_drawing_class_t fd_drawing_rectangle_class;

#endif
