#include "fd_drawing_text.h"

#include <string.h>

fd_drawing_text_buffer_implement(fd, 8)
fd_drawing_text_buffer_implement(fd, 16)
fd_drawing_text_buffer_implement(fd, 32)

void fd_drawing_text_layout(fd_drawing_text_t *drawing, fd_graphics_t *graphics) {
    const fd_view_text_t *view = &drawing->view;
    if (!view->visible) {
        drawing->area = fd_graphics_area_empty;
        drawing->origin = drawing->view.location;
        return;
    }
    
    fd_graphics_set_font(graphics, view->font.font);
    fd_graphics_metrics_t metrics = fd_graphics_get_string_metrics_length(graphics, view->string.string, view->string.length);
    drawing->area = metrics.bounds;
    fd_graphics_point_t location = drawing->view.location;
    switch (view->alignments.x) {
        case fd_view_alignment_origin:
            drawing->origin.x = location.x;
            break;
        case fd_view_alignment_min:
            drawing->origin.x = location.x - drawing->area.x;
            break;
        case fd_view_alignment_max:
            drawing->origin.x = location.x - (drawing->area.x + drawing->area.width);
            break;
        case fd_view_alignment_center:
            drawing->origin.x = location.x - (drawing->area.x + drawing->area.width / 2);
            break;
    }
    drawing->area.x += drawing->origin.x;
    switch (view->alignments.y) {
        case fd_view_alignment_origin:
            drawing->origin.y = location.y;
            break;
        case fd_view_alignment_min:
            drawing->origin.y = location.y - drawing->area.y;
            break;
        case fd_view_alignment_max:
            drawing->origin.y = location.y - (drawing->area.y + drawing->area.height);
            break;
        case fd_view_alignment_center:
            drawing->origin.y = location.y - (drawing->area.y + drawing->area.height / 2);
            break;
    }
    drawing->area.y += drawing->origin.y;
}

void fd_drawing_text_update(fd_drawing_update_parameters_t *parameters) {
    fd_drawing_text_t *drawing = (fd_drawing_text_t *)parameters->drawing->object;
    fd_view_text_t *old_view = &drawing->previous_view;
    fd_view_text_t *new_view = &drawing->view;
    if (memcmp(old_view, new_view, sizeof(*old_view)) == 0) {
        return;
    }
    
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_area_t old_area = drawing->area;
    fd_drawing_text_layout(drawing, context->graphics);
    fd_graphics_area_t new_area = drawing->area;
    fd_graphics_area_t area = fd_graphics_area_union(old_area, new_area);
// !!! we don't have an opaque text mode yet. -denis
//    bool opaque = fd_graphics_area_contains(new_area, old_area);
    bool opaque = false;
    parameters->add_change(context, area, opaque);
    memcpy(old_view, new_view, sizeof(*old_view));
    // copy text buffer if present
    if (drawing->update != NULL) {
        drawing->update(drawing);
    }
}

void fd_drawing_text_render(fd_drawing_render_parameters_t *parameters) {
    const fd_drawing_text_t *drawing = (fd_drawing_text_t *)parameters->drawing->object;
    const fd_view_text_t *view = &drawing->previous_view;
    if (!view->visible) {
        return;
    }
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_t *graphics = context->graphics;
    fd_graphics_set_font(graphics, view->font.font);
    fd_graphics_set_foreground(graphics, view->color);
    fd_graphics_write_string_length(graphics, drawing->origin.x, drawing->origin.y, view->string.string, view->string.length);
}

void fd_drawing_text_get_area(fd_drawing_get_area_parameters_t *parameters) {
    fd_drawing_text_t *drawing = (fd_drawing_text_t *)parameters->drawing->object;
    memcpy(&parameters->area, &drawing->area, sizeof(parameters->area));
}

fd_drawing_class_t fd_drawing_text_class = {
    .update = fd_drawing_text_update,
    .render = fd_drawing_text_render,
    .get_area = fd_drawing_text_get_area,
};
