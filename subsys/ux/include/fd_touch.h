#ifndef fd_touch_h
#define fd_touch_h

#include <stdint.h>

typedef enum {
    fd_touch_action_released = 0,
    fd_touch_action_pressed = 1,
} fd_touch_action_t;

typedef enum {
    fd_touch_gesture_none = 0x00,
    fd_touch_gesture_slide_up = 0x01,
    fd_touch_gesture_slide_down = 0x02,
    fd_touch_gesture_slide_left = 0x03,
    fd_touch_gesture_slide_right = 0x04,
    fd_touch_gesture_click = 0x05,
    fd_touch_gesture_double_click = 0x0B,
    fd_touch_gesture_press = 0x0C,
} fd_touch_gesture_t;

typedef struct {
    fd_touch_action_t action;
    fd_touch_gesture_t gesture;
    int x;
    int y;
} fd_touch_event_t;

#endif
