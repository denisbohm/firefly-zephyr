#ifndef fd_gc9a01_h
#define fd_gc9a01_h

#include <stddef.h>
#include <stdint.h>

void fd_gc9a01_initialize(void);

void fd_gc9a01_display_on(void);
void fd_gc9a01_display_off(void);

void fd_gc9a01_write_image_start(int x, int y, int width, int height);
void fd_gc9a01_write_image_subdata(const uint8_t *data, size_t size);
void fd_gc9a01_write_image_end(void);

#endif