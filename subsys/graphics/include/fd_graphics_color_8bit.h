#ifndef fd_graphics_color_8bit_h
#define fd_graphics_color_8bit_h

#include "fd_graphics.h"

#include <stddef.h>

typedef struct {
    fd_graphics_backend_t backend;
    uint8_t *frame_buffer;
    size_t frame_buffer_size;
} fd_graphics_color_8bit_t;

#define fd_graphics_color_8bit_frame_buffer_size(width, height) (3 * width * height)

void fd_graphics_color_8bit_initialize(
    fd_graphics_color_8bit_t *color_8bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
);

#endif