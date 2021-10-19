#ifndef fd_canvas_h
#define fd_canvas_h

#include "fd_drawing.h"

typedef struct {
    fd_graphics_area_t area;
    bool opaque;
} fd_canvas_change_t;

typedef struct {
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    
    fd_graphics_t *graphics;
    fd_canvas_change_t change;
} fd_canvas_t;

void fd_canvas_initialize(fd_canvas_t *canvas);

void fd_canvas_update(fd_canvas_t *canvas);
void fd_canvas_render(fd_canvas_t *canvas);

#endif
