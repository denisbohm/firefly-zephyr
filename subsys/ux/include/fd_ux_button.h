#ifndef fd_ux_button_h
#define fd_ux_button_h

#include "fd_button.h"
#include "fd_gpio.h"
#include "fd_timer.h"
#include "fd_ux.h"

#include <stdbool.h>
#include <stdint.h>

struct fd_ux_button_s;

typedef struct {
    fd_ux_t *ux;
    const fd_gpio_t *gpios;
    uint32_t count;
    void (*callback)(struct fd_ux_button_s *ux_button, const fd_button_event_t *event);
} fd_ux_button_configuration_t;

typedef struct {
    uint32_t press_timestamp;
    uint32_t release_timestamp;
    uint32_t chords;
    bool press_sent;
} fd_ux_button_state_t;

#ifndef CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT
#define CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT 2
#endif

typedef struct fd_ux_button_s {
    fd_ux_button_configuration_t configuration;
    fd_timer_t timer;
    fd_ux_listener_t listener;
    uint32_t buttons;
    fd_ux_button_state_t states[CONFIG_FIREFLY_SUBSYS_UX_BUTTON_LIMIT];
    bool consume_release;
} fd_ux_button_t;

void fd_ux_button_initialize(fd_ux_button_t *ux_button, const fd_ux_button_configuration_t *configuration);

void fd_ux_button_event(fd_ux_button_t *ux_button, const fd_button_event_t *event);

#endif
