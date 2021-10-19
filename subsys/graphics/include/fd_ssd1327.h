#ifndef fd_ssd1327_h
#define fd_ssd1327_h

#include "fd_gpio.h"

#include <stdint.h>

typedef struct {
    fd_gpio_t resx;
    fd_gpio_t csx;
    fd_gpio_t dcx;
} fd_ssd1327_configuration_t;

void fd_ssd1327_initialize(fd_ssd1327_configuration_t configuration);

void fd_ssd1327_display_on(void);
void fd_ssd1327_display_off(void);

void fd_ssd1327_write_image_start(int x, int y, int width, int height);
void fd_ssd1327_write_image_subdata(const uint8_t *data, int length);
void fd_ssd1327_write_image_end(void);

#endif
