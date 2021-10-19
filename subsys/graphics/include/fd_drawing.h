#ifndef fd_drawing_h
#define fd_drawing_h

#include "fd_graphics.h"

typedef struct {
    void *object;
    fd_graphics_t *graphics;
} fd_drawing_context_t;

typedef struct fd_drawing_s fd_drawing_t;

typedef struct {
    fd_drawing_t *drawing;
    fd_drawing_context_t *context;
    void (*add_change)(fd_drawing_context_t *context, fd_graphics_area_t area, bool opaque);
} fd_drawing_update_parameters_t;

typedef struct {
    fd_drawing_t *drawing;
    fd_drawing_context_t *context;
} fd_drawing_render_parameters_t;

typedef struct {
    fd_drawing_t *drawing;
    fd_drawing_context_t *context;
    fd_graphics_area_t area;
} fd_drawing_get_area_parameters_t;

typedef struct {
    void (*update)(fd_drawing_update_parameters_t *parameters);
    void (*render)(fd_drawing_render_parameters_t *parameters);
    void (*get_area)(fd_drawing_get_area_parameters_t *parameters);
} fd_drawing_class_t;

struct fd_drawing_s {
    void *object;
    fd_drawing_class_t *class;
};

typedef struct {
    fd_drawing_t *drawings;
    uint32_t drawing_count;
} fd_drawing_plane_t;

void fd_drawing_plane_add(fd_drawing_plane_t *plane, size_t size, void *object, fd_drawing_class_t *class);

#endif
