#ifndef fd_event_h
#define fd_event_h

#include <stdint.h>
#include <stdbool.h>

void fd_event_initialize(void);

uint32_t fd_event_get_identifier(const char *name);

typedef void (*fd_event_callback_t)(uint32_t identifier);

void fd_event_add_callback(uint32_t identifier, fd_event_callback_t callback);

void fd_event_set(uint32_t identifier);
void fd_event_set_from_interrupt(uint32_t identifier);

void fd_event_process(void);

#endif
