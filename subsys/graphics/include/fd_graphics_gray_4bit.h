#ifndef fd_graphics_gray_4bit_h
#define fd_graphics_gray_4bit_h

#include "fd_graphics.h"

#include <stddef.h>

typedef struct {
    fd_graphics_backend_t backend;
    uint8_t *frame_buffer;
    size_t frame_buffer_size;
} fd_graphics_gray_4bit_t;

#define fd_graphics_gray_4bit_frame_buffer_size(width, height) ((width / 2) * height)

void fd_graphics_gray_4bit_initialize(
    fd_graphics_gray_4bit_t *gray_4bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
);

#endif