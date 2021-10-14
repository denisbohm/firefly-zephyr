#include "fd_canvas.h"

#include <string.h>

void fd_canvas_add_change(fd_drawing_context_t *context, fd_graphics_area_t area, bool opaque) {
    fd_canvas_t *canvas = (fd_canvas_t *)context->object;
    canvas->change.area = fd_graphics_area_union(canvas->change.area, area);
    canvas->change.opaque = canvas->change.opaque && opaque;
}

void fd_canvas_update(fd_canvas_t *canvas) {
    canvas->change.area = fd_graphics_area_empty;
    canvas->change.opaque = true;
    fd_drawing_context_t context = {
        .object = canvas,
        .graphics = canvas->graphics,
    };
    fd_drawing_update_parameters_t update_parameters = {
        .context = &context,
        .add_change = fd_canvas_add_change,
    };
    for (uint32_t i = 0; i < canvas->plane_count; ++i) {
        fd_drawing_plane_t *plane = canvas->planes[i];
        for (uint32_t j = 0; j < plane->drawing_count; ++j) {
            fd_drawing_t *drawing = &plane->drawings[j];
            update_parameters.drawing = drawing;
            drawing->class->update(&update_parameters);
        }
    }
}

void fd_canvas_render(fd_canvas_t *canvas) {
    if (fd_graphics_area_is_empty(canvas->change.area)) {
        return;
    }
    
    fd_graphics_reset(canvas->graphics);
    fd_graphics_set_clipping(canvas->graphics, canvas->change.area);
    if (!canvas->change.opaque) {
        fd_graphics_set_foreground(canvas->graphics, (fd_graphics_color_t) { .argb = 0xff000000 });
        fd_graphics_write_area(canvas->graphics, canvas->change.area);
    }
    fd_graphics_set_background(canvas->graphics, (fd_graphics_color_t) { .argb = 0xff000000 });
    fd_graphics_set_foreground(canvas->graphics, (fd_graphics_color_t) { .argb = 0xffffffff });
    fd_drawing_context_t context = {
        .object = canvas,
        .graphics = canvas->graphics,
    };
    fd_drawing_render_parameters_t render_parameters = {
        .context = &context,
    };
    fd_drawing_get_area_parameters_t get_area_parameters = {
        .context = &context,
    };
    for (uint32_t i = 0; i < canvas->plane_count; ++i) {
        fd_drawing_plane_t *plane = canvas->planes[i];
        for (uint32_t j = 0; j < plane->drawing_count; ++j) {
            fd_drawing_t *drawing = &plane->drawings[j];
            get_area_parameters.drawing = drawing;
            drawing->class->get_area(&get_area_parameters);
            if (!fd_graphics_area_intersects(canvas->change.area, get_area_parameters.area)) {
                continue;
            }
            render_parameters.drawing = drawing;
            drawing->class->render(&render_parameters);
        }
    }
    fd_graphics_reset(canvas->graphics);
}

void fd_canvas_initialize(fd_canvas_t *canvas) {
    memset(canvas, 0, sizeof(*canvas));
}
