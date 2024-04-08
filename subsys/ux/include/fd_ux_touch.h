#ifndef fd_ux_touch_h
#define fd_ux_touch_h

#include "fd_touch.h"
#include "fd_ux.h"

#include <stdbool.h>
#include <stdint.h>

struct fd_ux_touch_s;

typedef struct {
    uint32_t identifier;
    fd_ux_t *ux;
    void (*callback)(struct fd_ux_touch_s *ux_touch, const fd_touch_event_t *event);
} fd_ux_touch_configuration_t;

typedef struct fd_ux_touch_s {
    fd_ux_touch_configuration_t configuration;
    fd_ux_listener_t listener;
    bool consume_release;
    uint32_t touchs;
} fd_ux_touch_t;

fd_ux_touch_t *fd_ux_touch_get(uint32_t identifier);

void fd_ux_touch_initialize(fd_ux_touch_t *ux_touch, const fd_ux_touch_configuration_t *configuration);

void fd_ux_touch_event(fd_ux_touch_t *ux_touch, const fd_touch_event_t *event);

#endif
