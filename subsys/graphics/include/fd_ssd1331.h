#ifndef fd_ssd1331_h
#define fd_ssd1331_h

#include <stddef.h>
#include <stdint.h>

void fd_ssd1331_initialize(void);

void fd_ssd1331_display_on(void);
void fd_ssd1331_display_off(void);

void fd_ssd1331_write_image_start(int x, int y, int width, int height);
void fd_ssd1331_write_image_subdata(const uint8_t *data, size_t size);
void fd_ssd1331_write_image_end(void);

#endif