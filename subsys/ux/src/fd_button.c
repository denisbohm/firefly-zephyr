#include "fd_button.h"

bool fd_button_was_pressed(const fd_button_event_t *event, uint32_t mask) {
    return (event->action == fd_button_action_pressed) && ((event->buttons & mask) != 0);
}

bool fd_button_was_released(const fd_button_event_t *event, uint32_t mask) {
    return (event->action == fd_button_action_released) && ((event->buttons & mask) != 0);
}

bool fd_button_was_pressed_exclusively(const fd_button_event_t *event, uint32_t mask) {
    return fd_button_was_pressed(event, mask) && (event->holds == 0) && (event->chords == 0);
}

bool fd_button_was_released_exclusively(const fd_button_event_t *event, uint32_t mask) {
    return fd_button_was_released(event, mask) && (event->holds == 0) && (event->chords == 0);
}
