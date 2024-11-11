#include "fd_canvas.h"

#include "fd_timing.h"

#include <string.h>

#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
typedef struct {
    bool initialized;
    fd_timing_t update_timing;
    fd_timing_t render_timing;
    fd_timing_t transfer_timing;
} fd_canvas_timing_t;

fd_canvas_timing_t fd_canvas_timing;
#endif

void fd_canvas_add_change(fd_drawing_context_t *context, fd_graphics_area_t area, bool opaque) {
    fd_canvas_t *canvas = (fd_canvas_t *)context->object;
    canvas->change.area = fd_graphics_area_union(canvas->change.area, area);
    canvas->change.opaque = canvas->change.opaque && opaque;
}

void fd_canvas_update(fd_canvas_t *canvas) {
#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    fd_timing_start(&fd_canvas_timing.update_timing);
#endif

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

#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    fd_timing_end(&fd_canvas_timing.update_timing);
#endif
}

void fd_canvas_render(fd_canvas_t *canvas) {
    fd_graphics_area_t area = canvas->change.area;
    if (fd_graphics_area_is_empty(area)) {
        return;
    }
    
#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    fd_timing_start(&fd_canvas_timing.render_timing);
#endif

    fd_graphics_reset(canvas->graphics);
    fd_graphics_set_clipping(canvas->graphics, area);
    if (!canvas->change.opaque) {
        fd_graphics_set_foreground(canvas->graphics, (fd_graphics_color_t) { .argb = 0xff000000 });
        fd_graphics_write_area(canvas->graphics, area);
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
            if (!fd_graphics_area_intersects(area, get_area_parameters.area)) {
                continue;
            }
            render_parameters.drawing = drawing;
            drawing->class->render(&render_parameters);
        }
    }

#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    fd_timing_end(&fd_canvas_timing.render_timing);

    fd_timing_start(&fd_canvas_timing.transfer_timing);
#endif

    fd_graphics_reset(canvas->graphics);
    fd_graphics_update(canvas->graphics, area);

#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    fd_timing_end(&fd_canvas_timing.transfer_timing);
#endif
}

void fd_canvas_initialize(fd_canvas_t *canvas) {
    memset(canvas, 0, sizeof(*canvas));

#if CONFIG_FIREFLY_SUBSYS_BASE_TIMING_NRF5
    if (!fd_canvas_timing.initialized) {
        fd_canvas_timing.initialized = true;
        fd_timing_register(&fd_canvas_timing.update_timing, "canvas update");
        fd_timing_register(&fd_canvas_timing.render_timing, "canvas render");
        fd_timing_register(&fd_canvas_timing.transfer_timing, "canvas transfer");
    }
#endif
}
