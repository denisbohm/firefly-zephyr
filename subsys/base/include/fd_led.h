#ifndef fd_led_h
#define fd_led_h

#include "fd_source.h"

#include <stdbool.h>
#include <stdint.h>

fd_source_push()

typedef struct {
    float r;
    float g;
    float b;
} fd_led_color_t;

typedef struct {
    fd_led_color_t zero;
    fd_led_color_t one;
    float (*map)(float t);
    void (*set)(const fd_led_color_t *color);
} fd_led_step_context_t;

fd_led_color_t fd_led_rgb_to_color(uint32_t rgb);

void fd_led_waypoint(void *context, float t);

fd_source_pop()

#endif
