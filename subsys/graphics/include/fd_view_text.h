#ifndef fd_view_text_h
#define fd_view_text_h

#include "fd_view.h"

typedef struct {
    bool visible;
    fd_graphics_point_t location;
    fd_view_alignments_t alignments;
    fd_view_resource_string_t string;
    fd_view_resource_font_t font;
    fd_graphics_color_t color;
} fd_view_text_t;

#endif
