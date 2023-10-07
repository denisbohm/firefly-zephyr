#ifndef fd_drawing_battery_h
#define fd_drawing_battery_h

#include "fd_view_battery.h"

#include "fd_drawing.h"

typedef struct {
    fd_view_battery_t view;
    
    fd_view_battery_t previous_view;
    fd_graphics_area_t area;
    fd_graphics_point_t origin;
} fd_drawing_battery_t;

extern fd_drawing_class_t fd_drawing_battery_class;

void fd_drawing_battery_initialize(fd_drawing_battery_t *battery);

#endif
