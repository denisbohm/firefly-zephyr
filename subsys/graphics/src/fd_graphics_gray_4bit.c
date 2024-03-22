#include "fd_graphics_gray_4bit.h"

#include <string.h>

fd_source_push_speed()

static uint32_t fd_graphics_gray_4bit_rgb_to_4bit_gray(uint32_t r, uint32_t g, uint32_t b) {
    uint32_t gray = ((r + g + b) / 3) >> 4;
    return gray;
}

static uint32_t fd_graphics_gray_4bit_color_to_4bit_gray(fd_graphics_color_t color) {
    return fd_graphics_gray_4bit_rgb_to_4bit_gray(color.r, color.g, color.b);
}

static fd_graphics_gray_4bit_t *fd_graphics_gray_4bit_impl(fd_graphics_t *graphics) {
    return (fd_graphics_gray_4bit_t *)graphics->impl;
}

static void fd_graphics_gray_4bit_set_pixel(fd_graphics_t *graphics, int lx, int ly, uint32_t gray) {
    int x = graphics->width - lx - 1;
    int y = graphics->height - ly - 1;
    uint32_t span = graphics->width / 2;
    uint32_t index = y * span + x / 2;
    uint8_t *frame_buffer = fd_graphics_gray_4bit_impl(graphics)->frame_buffer;
    uint32_t byte = frame_buffer[index];
    if (x & 1) {
        byte = (byte & 0xf0) | gray;
    } else {
        byte = (byte & 0x0f) | (gray << 4);
    }
    frame_buffer[index] = byte;
}

static void fd_graphics_gray_4bit_blit(fd_graphics_t *graphics, fd_graphics_area_t larea) {
}

static void fd_graphics_gray_4bit_write_background(fd_graphics_t *graphics) {
    fd_graphics_area_t area = { .x = 0, .y = 0, .width = graphics->width, .height = graphics->height };
    uint32_t gray = fd_graphics_gray_4bit_color_to_4bit_gray(graphics->background);
    uint8_t byte = (gray << 4) | gray;
    memset(fd_graphics_gray_4bit_impl(graphics)->frame_buffer, byte, sizeof(fd_graphics_gray_4bit_impl(graphics)->frame_buffer_size));

    fd_graphics_gray_4bit_blit(graphics, area);
}

static void fd_graphics_gray_4bit_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    uint32_t gray = fd_graphics_gray_4bit_color_to_4bit_gray(graphics->foreground);
    int dx = area.x;
    int dy = area.y;
    int width = area.width;
    int height = area.height;
    for (int cy = 0; cy < height; ++cy) {
        for (int cx = 0; cx < width; ++cx) {
            fd_graphics_gray_4bit_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_gray_4bit_blit(graphics, area);
}

static void fd_graphics_gray_4bit_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = image->width, .height = image->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int dx = dst_area.x;
    int dy = dst_area.y;
    int sx = dx - x;
    int sy = dy - y;
    for (int cy = 0; cy < dst_area.height; ++cy) {
        for (int cx = 0; cx < dst_area.width; ++cx) {
            uint8_t *src = fd_graphics_image_get_pixel(image, sx + cx, sy + cy);
            uint32_t r = *src++;
            uint32_t g = *src++;
            uint32_t b = *src++;
            uint32_t gray = fd_graphics_gray_4bit_rgb_to_4bit_gray(r, g, b);
            fd_graphics_gray_4bit_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_gray_4bit_blit(graphics, dst_area);
}

static void fd_graphics_gray_4bit_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
    int x = rx - bitmap->origin.x;
    int y = ry - bitmap->origin.y;
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = bitmap->width, .height = bitmap->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int dx = dst_area.x;
    int dy = dst_area.y;
    int sx = dx - x;
    int sy = dy - y;
    uint32_t fr = graphics->foreground.r;
    uint32_t fg = graphics->foreground.g;
    uint32_t fb = graphics->foreground.b;
    uint32_t sr = graphics->background.r;
    uint32_t sg = graphics->background.g;
    uint32_t sb = graphics->background.b;
    uint32_t max = (1 << bitmap->depth) - 1;
    for (int cy = 0; cy < dst_area.height; ++cy) {
        for (int cx = 0; cx < dst_area.width; ++cx) {
            int alpha = fd_graphics_bitmap_get_pixel(bitmap, sx + cx, sy + cy);
            int r = (fr * alpha + sr * (max - alpha)) / max;
            int g = (fg * alpha + sg * (max - alpha)) / max;
            int b = (fb * alpha + sb * (max - alpha)) / max;
            uint32_t gray = fd_graphics_gray_4bit_rgb_to_4bit_gray(r, g, b);
            fd_graphics_gray_4bit_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_gray_4bit_blit(graphics, dst_area);
}

void fd_graphics_gray_4bit_initialize(
    fd_graphics_gray_4bit_t *gray_4bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
) {
    graphics->impl = gray_4bit;
    gray_4bit->backend.write_background = fd_graphics_gray_4bit_write_background;
    gray_4bit->backend.write_area = fd_graphics_gray_4bit_write_area;
    gray_4bit->backend.write_image = fd_graphics_gray_4bit_write_image;
    gray_4bit->backend.write_bitmap = fd_graphics_gray_4bit_write_bitmap;
    gray_4bit->frame_buffer = frame_buffer;
    gray_4bit->frame_buffer_size = frame_buffer_size;
    fd_graphics_initialize(graphics, width, height, gray_4bit->backend, gray_4bit->frame_buffer);
}

fd_source_pop()