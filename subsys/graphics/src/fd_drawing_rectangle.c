#include "fd_drawing_rectangle.h"

#include "fd_unused.h"

#include <string.h>

void fd_drawing_rectangle_layout(fd_drawing_rectangle_t *drawing, fd_graphics_t *graphics fd_unused) {
    const fd_view_rectangle_t *view = &drawing->view;
    if (!view->visible) {
        drawing->area = fd_graphics_area_empty;
        return;
    }
    
    drawing->area = drawing->view.area;
    fd_graphics_point_t location = drawing->view.location;
    fd_graphics_point_t origin = fd_drawing_align(drawing->area, location, view->alignments);
    drawing->area.x += origin.x;
    drawing->area.y += origin.y;
}

void fd_drawing_rectangle_update(fd_drawing_update_parameters_t *parameters) {
    fd_drawing_rectangle_t *drawing = (fd_drawing_rectangle_t *)parameters->drawing->object;
    fd_view_rectangle_t *old_view = &drawing->previous_view;
    fd_view_rectangle_t *new_view = &drawing->view;
    if (memcmp(old_view, new_view, sizeof(*old_view)) == 0) {
        return;
    }
    
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_area_t old_area = drawing->area;
    fd_drawing_rectangle_layout(drawing, context->graphics);
    fd_graphics_area_t new_area = drawing->area;
    fd_graphics_area_t area = fd_graphics_area_union(old_area, new_area);
    bool opaque = fd_graphics_area_contains(new_area, old_area);
    parameters->add_change(context, area, opaque);
    memcpy(old_view, new_view, sizeof(*old_view));
}

void fd_drawing_rectangle_render(fd_drawing_render_parameters_t *parameters) {
    const fd_drawing_rectangle_t *drawing = (fd_drawing_rectangle_t *)parameters->drawing->object;
    const fd_view_rectangle_t *view = &drawing->previous_view;
    if (!view->visible) {
        return;
    }
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_t *graphics = context->graphics;
    fd_graphics_set_foreground(graphics, view->color);
    fd_graphics_write_area(graphics, drawing->area);
}

void fd_drawing_rectangle_get_area(fd_drawing_get_area_parameters_t *parameters) {
    fd_drawing_rectangle_t *drawing = (fd_drawing_rectangle_t *)parameters->drawing->object;
    memcpy(&parameters->area, &drawing->area, sizeof(parameters->area));
}

fd_drawing_class_t fd_drawing_rectangle_class = {
    .update = fd_drawing_rectangle_update,
    .render = fd_drawing_rectangle_render,
    .get_area = fd_drawing_rectangle_get_area,
};
