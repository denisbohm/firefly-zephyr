#include "fd_graphics_ssd1327.h"

#include "fd_graphics.h"
#include "fd_ssd1327.h"

#include <string.h>

// The SSD1327 is not pixel addressable.  Each byte transferred has 2 pixels.
// We use a full frame buffer so that we can transfer any pixels necessary that
// are not part of the actual graphics update area. -denis

typedef struct {
    fd_graphics_t graphics;
    uint8_t buffer[128 * 3];
    uint8_t frame_buffer[(128 / 2) * 128];
} fd_graphics_ssd1327_t;

static fd_graphics_ssd1327_t fd_graphics_ssd1327;

static uint32_t fd_graphics_ssd1327_rgb_to_4bit_gray(uint32_t r, uint32_t g, uint32_t b) {
    uint32_t gray = ((r + g + b) / 3) >> 4;
    return gray;
}

static uint32_t fd_graphics_ssd1327_color_to_4bit_gray(fd_graphics_color_t color) {
    return fd_graphics_ssd1327_rgb_to_4bit_gray(color.r, color.g, color.b);
}

static fd_graphics_ssd1327_t *fd_graphics_ssd1327_impl(fd_graphics_t *graphics) {
    return (fd_graphics_ssd1327_t *)graphics->impl;
}

static void fd_graphics_ssd1327_set_pixel(fd_graphics_t *graphics, int x, int y, uint32_t gray) {
    uint32_t span = graphics->width / 2;
    uint32_t index = y * span + x / 2;
    uint8_t *frame_buffer = fd_graphics_ssd1327_impl(graphics)->frame_buffer;
    uint32_t byte = frame_buffer[index];
    if (x & 1) {
        byte = (byte & 0xf0) | gray;
    } else {
        byte = (byte & 0x0f) | (gray << 4);
    }
    frame_buffer[index] = byte;
}

static void fd_graphics_ssd1327_blit(fd_graphics_t *graphics, fd_graphics_area_t area) {
    // blit an area from the frame buffer that starts and ends on an even x boundary
    int r = (area.x + area.width + 1) & ~1;
    int x = area.x & ~1;
    int width = r - x;
    int y = area.y;
    int height = area.height;
    fd_ssd1327_write_image_start(x, y, width, height);
    uint32_t span = graphics->width / 2;
    uint8_t *data = &fd_graphics_ssd1327_impl(graphics)->frame_buffer[y * span + x / 2];
    uint32_t sub_span = width / 2;
    for (int i = 0; i < height; ++i) {
        fd_ssd1327_write_image_subdata(data, sub_span);
        data += span;
    }
    fd_ssd1327_write_image_end();
}

static void fd_graphics_ssd1327_write_background(fd_graphics_t *graphics) {
    fd_graphics_area_t area = { .x = 0, .y = 0, .width = graphics->width, .height = graphics->height };
    uint32_t gray = fd_graphics_ssd1327_color_to_4bit_gray(graphics->background);
    uint8_t byte = (gray << 4) | gray;
    memset(fd_graphics_ssd1327_impl(graphics)->frame_buffer, byte, sizeof(fd_graphics_ssd1327_impl(graphics)->frame_buffer));

    fd_graphics_ssd1327_blit(graphics, area);
}

static void fd_graphics_ssd1327_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    uint32_t gray = fd_graphics_ssd1327_color_to_4bit_gray(graphics->foreground);
    int dx = area.x;
    int dy = area.y;
    int width = area.width;
    int height = area.height;
    for (int cy = 0; cy < height; ++cy) {
        for (int cx = 0; cx < width; ++cx) {
            fd_graphics_ssd1327_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_ssd1327_blit(graphics, area);
}

static void fd_graphics_ssd1327_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
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
            uint32_t gray = fd_graphics_ssd1327_rgb_to_4bit_gray(r, g, b);
            fd_graphics_ssd1327_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_ssd1327_blit(graphics, dst_area);
}

static void fd_graphics_ssd1327_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
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
            uint32_t gray = fd_graphics_ssd1327_rgb_to_4bit_gray(r, g, b);
            fd_graphics_ssd1327_set_pixel(graphics, dx + cx, dy + cy, gray);
        }
    }

    fd_graphics_ssd1327_blit(graphics, dst_area);
}

fd_graphics_t *fd_graphics_ssd1327_get(void) {
    return &fd_graphics_ssd1327.graphics;
}

void fd_graphics_ssd1327_initialize(void) {
    memset(&fd_graphics_ssd1327, 0, sizeof(fd_graphics_ssd1327));
    fd_graphics_ssd1327.graphics.impl = &fd_graphics_ssd1327;

    fd_graphics_backend_t backend = {
        .write_background = fd_graphics_ssd1327_write_background,
        .write_area = fd_graphics_ssd1327_write_area,
        .write_image = fd_graphics_ssd1327_write_image,
        .write_bitmap = fd_graphics_ssd1327_write_bitmap,
    };
    fd_graphics_initialize(&fd_graphics_ssd1327.graphics, 128, 128, backend, fd_graphics_ssd1327.buffer);
}
