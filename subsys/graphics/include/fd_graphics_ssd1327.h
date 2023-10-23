#ifndef fd_graphics_ssd1327_h
#define fd_graphics_ssd1327_h

#include "fd_graphics.h"

void fd_graphics_ssd1327_initialize(void);

fd_graphics_t *fd_graphics_ssd1327_get(void);

uint8_t *fd_graphics_ssd1327_frame_buffer(void);

#endif
