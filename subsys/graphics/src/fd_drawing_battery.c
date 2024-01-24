#include "fd_drawing_battery.h"

#include "fd_bitmap_battery.h"
#include "fd_bitmap_charging.h"
#include "fd_bitmap_powered.h"

#include "fd_assert.h"
#include "fd_canvas.h"
#include "fd_unused.h"

#include <math.h>
#include <string.h>

#define fd_drawing_battery_spacing 2

void fd_drawing_battery_initialize(fd_drawing_battery_t *battery) {
    memset(battery, 0, sizeof(*battery));
}

void fd_drawing_battery_layout(fd_drawing_battery_t *drawing, fd_graphics_t *graphics fd_unused) {
    const fd_view_battery_t *view = &drawing->view;
    if (!view->visible) {
        drawing->area = fd_graphics_area_empty;
        drawing->origin = drawing->view.location;
        return;
    }
    
    drawing->area = (fd_graphics_area_t) {
        .x = 0,
        .y = 0,
        .width =  fd_bitmap_charging.width + fd_drawing_battery_spacing + fd_bitmap_battery.width,
        .height = fd_bitmap_battery.height,
    };
    fd_graphics_point_t location = drawing->view.location;
    fd_graphics_point_t origin = fd_drawing_align(drawing->area, location, view->alignments);
    drawing->area.x = origin.x;
    drawing->area.y = origin.y;
}

void fd_drawing_battery_update(fd_drawing_update_parameters_t *parameters) {
    fd_drawing_battery_t *drawing = (fd_drawing_battery_t *)parameters->drawing->object;
    fd_view_battery_t *old_view = &drawing->previous_view;
    fd_view_battery_t *new_view = &drawing->view;
    if (memcmp(old_view, new_view, sizeof(*old_view)) == 0) {
        return;
    }
    
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_area_t old_area = drawing->area;
    fd_drawing_battery_layout(drawing, context->graphics);
    fd_graphics_area_t new_area = drawing->area;
    fd_graphics_area_t area = fd_graphics_area_union(old_area, new_area);
    bool opaque = false; // fd_graphics_area_contains(new_area, old_area); -can do this if just bar is updating...
    parameters->add_change(context, area, opaque);
    memcpy(old_view, new_view, sizeof(*old_view));
}

void fd_drawing_battery_render(fd_drawing_render_parameters_t *parameters) {
    const fd_drawing_battery_t *drawing = (fd_drawing_battery_t *)parameters->drawing->object;
    const fd_view_battery_t *view = &drawing->view;
    if (!view->visible) {
        return;
    }
    fd_drawing_context_t *context = parameters->context;
    fd_graphics_t *graphics = context->graphics;
    fd_graphics_set_foreground(graphics, view->color);
    fd_graphics_write_bitmap(
        graphics,
        drawing->area.x + fd_bitmap_charging.width + fd_drawing_battery_spacing,
        drawing->area.y,
        &fd_bitmap_battery
    );
    fd_graphics_area_t area = drawing->area;
    float state_of_charge = view->state_of_charge;
    int width = (int)round(state_of_charge * (float)(fd_bitmap_battery.width - 11));
    fd_graphics_area_t bar = {
        .x = drawing->area.x + fd_bitmap_charging.width + fd_drawing_battery_spacing + 4,
        .y = drawing->area.y + 4,
        .width = width,
        .height = area.height - 8,
    };
    fd_graphics_write_area(graphics, bar);

    int y = drawing->area.y + drawing->area.height / 2;
    if (view->status == fd_view_battery_status_charging) {
        fd_graphics_write_bitmap(graphics, drawing->area.x, y, &fd_bitmap_charging);
    } else
    if (view->status == fd_view_battery_status_powered) {
        fd_graphics_write_bitmap(graphics, drawing->area.x, y, &fd_bitmap_powered);
    }
}

void fd_drawing_battery_get_area(fd_drawing_get_area_parameters_t *parameters) {
    fd_drawing_battery_t *drawing = (fd_drawing_battery_t *)parameters->drawing->object;
    memcpy(&parameters->area, &drawing->area, sizeof(parameters->area));
}

fd_drawing_class_t fd_drawing_battery_class = {
    .update = fd_drawing_battery_update,
    .render = fd_drawing_battery_render,
    .get_area = fd_drawing_battery_get_area,
};
