#ifndef fd_view_battery_h
#define fd_view_battery_h

#include "fd_view.h"

typedef enum {
    fd_view_battery_status_unpowered,
    fd_view_battery_status_powered,
    fd_view_battery_status_charging,
} fd_view_battery_status_t;

typedef struct {
    bool visible;
    fd_graphics_point_t location;
    fd_view_alignments_t alignments;
    float state_of_charge;
    fd_view_battery_status_t status;
    fd_graphics_color_t color;
} fd_view_battery_t;

#endif
