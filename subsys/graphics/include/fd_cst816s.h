#ifndef fd_cst816s_h
#define fd_cst816s_h

#include <zephyr/kernel.h>

#include <stddef.h>
#include <stdint.h>

typedef enum {
    fd_cst816s_gesture_none = 0x00,
    fd_cst816s_gesture_slide_up = 0x01,
    fd_cst816s_gesture_slide_down = 0x02,
    fd_cst816s_gesture_slide_left = 0x03,
    fd_cst816s_gesture_slide_right = 0x04,
    fd_cst816s_gesture_click = 0x05,
    fd_cst816s_gesture_double_click = 0x0B,
    fd_cst816s_gesture_press = 0x0C,
} fd_cst816s_gesture_t;

typedef enum {
    fd_cst816s_finger_release = 0,
    fd_cst816s_finger_pressed = 1,
} fd_cst816s_finger_t;

typedef struct {
    uint8_t gesture;
    uint8_t finger;
    uint16_t x;
    uint16_t y;
} fd_cst816s_touch_t;

typedef struct {
    struct k_work_q *work_queue;
    void (*touch)(const fd_cst816s_touch_t *touch);
} fd_cst816s_configuration_t;

void fd_cst816s_initialize(const fd_cst816s_configuration_t *configuration);

#endif