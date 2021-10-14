#ifndef fd_ssd1327_h
#define fd_ssd1327_h

#include <stdint.h>

void fd_ssd1327_initialize(void);

void fd_ssd1327_display_on(void);
void fd_ssd1327_display_off(void);

void fd_ssd1327_write_image_start(int x, int y, int width, int height);
void fd_ssd1327_write_image_subdata(const uint8_t *data, int length);
void fd_ssd1327_write_image_end(void);

#endif
