#ifndef fd_ux_h
#define fd_ux_h

#include "fd_drawing.h"
#include "fd_graphics.h"
#include "fd_ux_button.h"

typedef struct {
    uint32_t id;
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    void (*preview)(void);
    void (*activate)(void);
    void (*deactivate)(void);
    bool (*button)(const fd_ux_button_event_t *event);
    bool (*animate)(void);
} fd_ux_screen_t;

typedef struct {
    fd_graphics_t *graphics;
    fd_ux_screen_t *screens;
    uint32_t screen_count;
    uint32_t initial_screen;
    uint32_t idle_ticks;
    void (*idle)(bool idle);
} fd_ux_configuration_t;

void fd_ux_initialize(fd_ux_configuration_t *configuration);

void fd_ux_tick(void);
void fd_ux_button_event(const fd_ux_button_event_t *event);

bool fd_ux_get_update_enabled(void);
void fd_ux_set_update_enabled(bool enable);

bool fd_ux_get_idle(void);
void fd_ux_set_idle(bool idle);

void fd_ux_set_animation(bool animation);

void fd_ux_set_screen(uint32_t id);
void fd_ux_set_screen_preview(uint32_t id);

#endif
