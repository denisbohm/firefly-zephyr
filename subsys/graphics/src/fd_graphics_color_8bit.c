#include "fd_graphics_color_8bit.h"

#include <string.h>

static fd_graphics_color_8bit_t *fd_graphics_color_8bit_impl(fd_graphics_t *graphics) {
    return (fd_graphics_color_8bit_t *)graphics->impl;
}

static void fd_graphics_color_8bit_set_pixel(fd_graphics_t *graphics, int lx, int ly, fd_graphics_color_t color) {
    int x = graphics->width - lx - 1;
    int y = graphics->height - ly - 1;
    uint32_t span = graphics->width;
    uint32_t index = 3 * (y * span + x);
    uint8_t *frame_buffer = fd_graphics_color_8bit_impl(graphics)->frame_buffer;
    frame_buffer[index++] = color.r;
    frame_buffer[index++] = color.g;
    frame_buffer[index++] = color.b;
}

static void fd_graphics_color_8bit_blit(fd_graphics_t *graphics, fd_graphics_area_t larea) {
}

static void fd_graphics_color_8bit_write_background(fd_graphics_t *graphics) {
    fd_graphics_area_t area = { .x = 0, .y = 0, .width = graphics->width, .height = graphics->height };
    fd_graphics_color_t color = graphics->background;
    uint8_t *frame_buffer = fd_graphics_color_8bit_impl(graphics)->frame_buffer;
    int count = graphics->width * graphics->height;
    for (int i = 0; i < count; ++i) {
        *frame_buffer++ = color.r;
        *frame_buffer++ = color.g;
        *frame_buffer++ = color.b;
    }

    fd_graphics_color_8bit_blit(graphics, area);
}

static void fd_graphics_color_8bit_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    fd_graphics_color_t color = graphics->foreground;
    int dx = area.x;
    int dy = area.y;
    int width = area.width;
    int height = area.height;
    for (int cy = 0; cy < height; ++cy) {
        for (int cx = 0; cx < width; ++cx) {
            fd_graphics_color_8bit_set_pixel(graphics, dx + cx, dy + cy, color);
        }
    }

    fd_graphics_color_8bit_blit(graphics, area);
}

static void fd_graphics_color_8bit_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = image->width, .height = image->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int dx = dst_area.x;
    int dy = dst_area.y;
    int sx = dx - x;
    int sy = dy - y;
    for (int cy = 0; cy < dst_area.height; ++cy) {
        for (int cx = 0; cx < dst_area.width; ++cx) {
            uint8_t *src = fd_graphics_image_get_pixel(image, sx + cx, sy + cy);
            uint8_t r = *src++;
            uint8_t g = *src++;
            uint8_t b = *src++;
            fd_graphics_color_8bit_set_pixel(graphics, dx + cx, dy + cy, (fd_graphics_color_t){ .r = r, .g = g, .b = b });
        }
    }

    fd_graphics_color_8bit_blit(graphics, dst_area);
}

static void fd_graphics_color_8bit_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
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
            uint8_t r = (uint8_t)((fr * alpha + sr * (max - alpha)) / max);
            uint8_t g = (uint8_t)((fg * alpha + sg * (max - alpha)) / max);
            uint8_t b = (uint8_t)((fb * alpha + sb * (max - alpha)) / max);
            fd_graphics_color_8bit_set_pixel(graphics, dx + cx, dy + cy, (fd_graphics_color_t){ .r = r, .g = g, .b = b });
        }
    }

    fd_graphics_color_8bit_blit(graphics, dst_area);
}

void fd_graphics_color_8bit_initialize(
    fd_graphics_color_8bit_t *color_8bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
) {
    graphics->impl = color_8bit;
    color_8bit->backend.write_background = fd_graphics_color_8bit_write_background;
    color_8bit->backend.write_area = fd_graphics_color_8bit_write_area;
    color_8bit->backend.write_image = fd_graphics_color_8bit_write_image;
    color_8bit->backend.write_bitmap = fd_graphics_color_8bit_write_bitmap;
    color_8bit->frame_buffer = frame_buffer;
    color_8bit->frame_buffer_size = frame_buffer_size;
    fd_graphics_initialize(graphics, width, height, color_8bit->backend, color_8bit->frame_buffer);
}