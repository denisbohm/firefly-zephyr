#include "fd_bitmap_powered.h"

const uint8_t fd_bitmap_powered_data[] = {
 0x02, 0x77, 0x20,
 0x2e, 0xff, 0xe2,
 0x7f, 0xff, 0xf7,
 0x6f, 0xff, 0xf6,
 0x2e, 0xff, 0xe2,
 0x02, 0x77, 0x20,
};

const fd_graphics_bitmap_t fd_bitmap_powered = {
    .width = 6,
    .height = 6,
    .depth = 4,
    .origin = { .x = 0, .y = 3 },
    .data = fd_bitmap_powered_data,
};
