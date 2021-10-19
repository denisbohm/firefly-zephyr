#ifndef fd_ili9341_h
#define fd_ili9341_h

#include <stdint.h>

void fd_ili9341_initialize(void);

void fd_ili9341_display_on(void);
void fd_ili9341_display_off(void);

void fd_ili9341_write_image_start(int x, int y, int width, int height);
void fd_ili9341_write_image_subdata(const uint8_t *data, int length);
void fd_ili9341_write_image_end(void);

#endif
