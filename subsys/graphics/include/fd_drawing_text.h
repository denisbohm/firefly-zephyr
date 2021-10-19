#ifndef fd_drawing_text_h
#define fd_drawing_text_h

#include "fd_drawing.h"
#include "fd_view_text.h"

typedef struct {
    fd_view_text_t view;
    
    fd_view_text_t previous_view;
    fd_graphics_area_t area;
    fd_graphics_point_t origin;
} fd_drawing_text_t;

extern fd_drawing_class_t fd_drawing_text_class;

#endif
