#ifndef fd_graphics_color_16bit_h
#define fd_graphics_color_16bit_h

#include "fd_graphics.h"

#include <stddef.h>

fd_source_push()

typedef struct {
    fd_graphics_backend_t backend;
    uint8_t *frame_buffer;
    size_t frame_buffer_size;
} fd_graphics_color_16bit_t;

#define fd_graphics_color_16bit_frame_buffer_size(width, height) (2 * width * height)

void fd_graphics_color_16bit_initialize(
    fd_graphics_color_16bit_t *color_16bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
);

fd_source_pop()

#endif