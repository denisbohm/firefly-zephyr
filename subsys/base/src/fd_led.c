#include "fd_led.h"

#include "fd_animation.h"

fd_source_push()

fd_led_color_t fd_led_rgb_to_color(uint32_t rgb) {
    fd_led_color_t color;
    color.r = (rgb >> 16) / 255.0f;
    color.g = (rgb >> 8) / 255.0f;
    color.b = rgb / 255.0f;
    return color;
}

float fd_led_interpolate(float a, float b, float f) {
    return (a + (b - a) * f);
}

void fd_led_waypoint(void *context, float t) {
    const fd_led_step_context_t *led_step_context = (const fd_led_step_context_t *)context;
    if (led_step_context->set == NULL) {
        return;
    }
    fd_led_color_t zero = led_step_context->zero;
    fd_led_color_t one = led_step_context->one;
    float f = led_step_context->map(t);
    fd_led_color_t color = {
        .r = fd_led_interpolate(zero.r, one.r, f),
        .g = fd_led_interpolate(zero.g, one.g, f),
        .b = fd_led_interpolate(zero.b, one.b, f),
    };
    led_step_context->set(&color);
}

fd_source_pop()
