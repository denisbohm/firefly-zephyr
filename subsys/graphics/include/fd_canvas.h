#ifndef fd_canvas_h
#define fd_canvas_h

#include "fd_drawing.h"

#include <zephyr/kernel.h>

typedef struct {
    fd_graphics_area_t area;
    bool opaque;
} fd_canvas_change_t;

typedef struct {
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    fd_canvas_change_t change;
} fd_canvas_render_state_t;

typedef struct {
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    
    fd_graphics_t *graphics;
    fd_canvas_change_t change;

    struct k_work_q *work_queue;
    struct k_work render_work;
    bool is_rendering;
    fd_canvas_render_state_t render_state;
} fd_canvas_t;

void fd_canvas_initialize(fd_canvas_t *canvas);

void fd_canvas_update(fd_canvas_t *canvas, fd_graphics_area_t area, bool opaque);

#endif
