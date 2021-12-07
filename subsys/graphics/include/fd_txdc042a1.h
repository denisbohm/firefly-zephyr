#ifndef fd_txdc042a1_h
#define fd_txdc042a1_h

#include "fd_gpio.h"

#include <stdint.h>

typedef struct {
    fd_gpio_t resx;
    fd_gpio_t csx;
    fd_gpio_t dcx;
    fd_gpio_t busy;
} fd_txdc042a1_configuration_t;

void fd_txdc042a1_initialize(fd_txdc042a1_configuration_t configuration);

void fd_txdc042a1_write_image_start(int x, int y, int width, int height);
void fd_txdc042a1_write_image_subdata(const uint8_t *data, int length);
void fd_txdc042a1_write_image_end(void);

#endif
