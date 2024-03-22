#include "fd_graphics_color_16bit.h"

#include <string.h>

fd_source_push_speed()

typedef union {
    uint16_t word;
    struct {
        uint16_t b:5;
        uint16_t g:6;
        uint16_t r:5;
    };
} fd_graphics_color_16bit_rgb_t;

static fd_graphics_color_16bit_t *fd_graphics_color_16bit_impl(fd_graphics_t *graphics) {
    return (fd_graphics_color_16bit_t *)graphics->impl;
}

static void fd_graphics_color_16bit_set_pixel(fd_graphics_t *graphics, int x, int y, fd_graphics_color_t color) {
    uint32_t span = graphics->width;
    uint32_t index = 2 * (y * span + x);
    uint8_t *frame_buffer = fd_graphics_color_16bit_impl(graphics)->frame_buffer;
    fd_graphics_color_16bit_rgb_t rgb = {
        .r = color.r >> 3,
        .g = color.g >> 2,
        .b = color.b >> 3,
    };
    frame_buffer[index++] = rgb.word >> 8;
    frame_buffer[index++] = rgb.word;
}

static void fd_graphics_color_16bit_get_pixel(fd_graphics_t *graphics, int x, int y, fd_graphics_color_t *color) {
    uint32_t span = graphics->width;
    uint32_t index = 2 * (y * span + x);
    uint8_t *frame_buffer = fd_graphics_color_16bit_impl(graphics)->frame_buffer;
    uint8_t b1 = frame_buffer[index++];
    uint8_t b0 = frame_buffer[index++];
    fd_graphics_color_16bit_rgb_t rgb = {
        .word = (b1 << 8) | b0,
    };
    color->r = rgb.r << 3;
    color->g = rgb.g << 2;
    color->b = rgb.b >> 3;
}

static void fd_graphics_color_16bit_blit(fd_graphics_t *graphics, fd_graphics_area_t area) {
}

static void fd_graphics_color_16bit_write_background(fd_graphics_t *graphics) {
    fd_graphics_area_t area = { .x = 0, .y = 0, .width = graphics->width, .height = graphics->height };
    fd_graphics_color_t color = graphics->background;
    fd_graphics_color_16bit_rgb_t rgb = {
        .r = color.r >> 3,
        .g = color.g >> 2,
        .b = color.b >> 3,
    };
    uint8_t b0 = rgb.word;
    uint8_t b1 = rgb.word >> 8;
    uint8_t *frame_buffer = fd_graphics_color_16bit_impl(graphics)->frame_buffer;
    int count = graphics->width * graphics->height;
    for (int i = 0; i < count; ++i) {
        *frame_buffer++ = b0;
        *frame_buffer++ = b1;
    }

    fd_graphics_color_16bit_blit(graphics, area);
}

static void fd_graphics_color_16bit_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    fd_graphics_color_t color = graphics->foreground;
    int dx = area.x;
    int dy = area.y;
    int width = area.width;
    int height = area.height;
    for (int cy = 0; cy < height; ++cy) {
        for (int cx = 0; cx < width; ++cx) {
            fd_graphics_color_16bit_set_pixel(graphics, dx + cx, dy + cy, color);
        }
    }

    fd_graphics_color_16bit_blit(graphics, area);
}

static void fd_graphics_color_16bit_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = image->width, .height = image->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int dx = dst_area.x;
    int dy = dst_area.y;
    int sx = dx - x;
    int sy = dy - y;
    uint32_t max = (1 << 8) - 1;
    for (int cy = 0; cy < dst_area.height; ++cy) {
        for (int cx = 0; cx < dst_area.width; ++cx) {
            uint8_t *src = fd_graphics_image_get_pixel_argb(image, sx + cx, sy + cy);
            uint8_t alpha = *src++;
            uint8_t sr = *src++;
            uint8_t sg = *src++;
            uint8_t sb = *src++;
            fd_graphics_color_t color;
            fd_graphics_color_16bit_get_pixel(graphics, dx + cx, dy + cy, &color);
            uint8_t r = (uint8_t)((sr * alpha + color.r * (max - alpha)) / max);
            uint8_t g = (uint8_t)((sg * alpha + color.g * (max - alpha)) / max);
            uint8_t b = (uint8_t)((sb * alpha + color.b * (max - alpha)) / max);
            fd_graphics_color_16bit_set_pixel(graphics, dx + cx, dy + cy, (fd_graphics_color_t){ .r = r, .g = g, .b = b });
        }
    }

    fd_graphics_color_16bit_blit(graphics, dst_area);
}

static void fd_graphics_color_16bit_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
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
    uint32_t max = (1 << bitmap->depth) - 1;
    for (int cy = 0; cy < dst_area.height; ++cy) {
        for (int cx = 0; cx < dst_area.width; ++cx) {
            int alpha = fd_graphics_bitmap_get_pixel(bitmap, sx + cx, sy + cy);
            fd_graphics_color_t color;
            fd_graphics_color_16bit_get_pixel(graphics, dx + cx, dy + cy, &color);
            uint8_t r = (uint8_t)((fr * alpha + color.r * (max - alpha)) / max);
            uint8_t g = (uint8_t)((fg * alpha + color.g * (max - alpha)) / max);
            uint8_t b = (uint8_t)((fb * alpha + color.b * (max - alpha)) / max);
            fd_graphics_color_16bit_set_pixel(graphics, dx + cx, dy + cy, (fd_graphics_color_t){ .r = r, .g = g, .b = b });
        }
    }

    fd_graphics_color_16bit_blit(graphics, dst_area);
}

void fd_graphics_color_16bit_initialize(
    fd_graphics_color_16bit_t *color_16bit,
    uint8_t *frame_buffer,
    size_t frame_buffer_size,
    fd_graphics_t *graphics,
    int width,
    int height
) {
    graphics->impl = color_16bit;
    color_16bit->backend.write_background = fd_graphics_color_16bit_write_background;
    color_16bit->backend.write_area = fd_graphics_color_16bit_write_area;
    color_16bit->backend.write_image = fd_graphics_color_16bit_write_image;
    color_16bit->backend.write_bitmap = fd_graphics_color_16bit_write_bitmap;
    color_16bit->frame_buffer = frame_buffer;
    color_16bit->frame_buffer_size = frame_buffer_size;
    fd_graphics_initialize(graphics, width, height, color_16bit->backend, color_16bit->frame_buffer);
}

fd_source_pop()