#ifndef fd_drawing_icon_h
#define fd_drawing_icon_h

#include "fd_drawing.h"
#include "fd_view_icon.h"

typedef struct {
    fd_view_icon_t view;
    
    fd_view_icon_t previous_view;
    fd_graphics_area_t area;
    fd_graphics_point_t origin;
} fd_drawing_icon_t;

extern fd_drawing_class_t fd_drawing_icon_class;

#endif
