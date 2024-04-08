#ifndef fd_button_h
#define fd_button_h

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fd_button_action_released = 0,
    fd_button_action_pressed = 1,
} fd_button_action_t;

typedef struct {
    fd_button_action_t action;
    uint32_t buttons;
    uint32_t holds;
    uint32_t chords;
    float timestamp;
    float duration;
} fd_button_event_t;

bool fd_button_was_pressed(const fd_button_event_t *event, uint32_t mask);
bool fd_button_was_released(const fd_button_event_t *event, uint32_t mask);

bool fd_button_was_pressed_exclusively(const fd_button_event_t *event, uint32_t mask);
bool fd_button_was_released_exclusively(const fd_button_event_t *event, uint32_t mask);

#endif
