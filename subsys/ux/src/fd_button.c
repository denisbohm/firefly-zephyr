#include "fd_button.h"

bool fd_button_was_pressed(const fd_button_event_t *event, uint32_t mask) {
    return (event->type == fd_button_type_pressed) && ((event->buttons & mask) != 0);
}

bool fd_button_was_released(const fd_button_event_t *event, uint32_t mask) {
    return (event->type == fd_button_type_released) && ((event->buttons & mask) != 0);
}
