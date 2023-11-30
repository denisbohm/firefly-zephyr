#ifndef fd_view_h
#define fd_view_h

#include "fd_graphics.h"

#include <stddef.h>

typedef enum {
    fd_view_alignment_origin,
    fd_view_alignment_min,
    fd_view_alignment_max,
    fd_view_alignment_center,
} fd_view_alignment_t;

typedef struct {
    fd_view_alignment_t x;
    fd_view_alignment_t y;
} fd_view_alignments_t;

typedef struct {
    uint32_t revision;
    const char *string;
    size_t length;
    size_t size;
} fd_view_resource_string_t;

typedef struct {
    uint32_t revision;
    const fd_graphics_font_t *font;
} fd_view_resource_font_t;

typedef struct {
    uint32_t revision;
    const fd_graphics_bitmap_t *bitmap;
} fd_view_resource_bitmap_t;

typedef struct {
    uint32_t revision;
    const fd_graphics_image_t *image;
} fd_view_resource_image_t;

void fd_view_string_update(fd_view_resource_string_t *resource, const char *string);
void fd_view_string_update_int(fd_view_resource_string_t *resource, int value);
void fd_view_string_update_int_suffix(fd_view_resource_string_t *resource, int value, const char *suffix);

void fd_view_string_update_reference(fd_view_resource_string_t *resource, const char *string);

#endif
