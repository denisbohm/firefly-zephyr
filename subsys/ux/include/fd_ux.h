#ifndef fd_ux_h
#define fd_ux_h

#include "fd_button.h"
#include "fd_canvas.h"
#include "fd_drawing.h"
#include "fd_graphics.h"
#include "fd_touch.h"

#include "ux.pb.h"

typedef enum {
    fd_ux_state_on,
    fd_ux_state_idle,
    fd_ux_state_off,
} fd_ux_state_t;

typedef struct {
    uint32_t id;
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    void (*preview)(void);
    void (*activate)(void);
    void (*deactivate)(void);
    bool (*animate)(void);
    bool (*button)(const fd_button_event_t *event);
    bool (*touch)(const fd_touch_event_t *event);
} fd_ux_screen_t;

typedef struct {
    void *context;
    void (*state_changed)(void *context, fd_ux_state_t old_state, fd_ux_state_t new_state);
} fd_ux_listener_t;

typedef struct {
    uint32_t identifier;
    fd_graphics_t *graphics;
    fd_ux_screen_t *screens;
    uint32_t screen_count;
    uint32_t initial_screen;
    uint32_t idle_ticks;
    void (*state_changed)(fd_ux_state_t old_state, fd_ux_state_t new_state);
    fd_drawing_plane_t *plane;
} fd_ux_configuration_t;

typedef struct {
    fd_ux_configuration_t configuration;
    char display_configuration_key[32];
    char interaction_configuration_key[32];
    firefly_ux_v1_DisplayConfiguration display_configuration;
    firefly_ux_v1_InteractionConfiguration interaction_configuration;
    const fd_ux_listener_t *listeners[4];
    uint32_t listener_count;
    fd_canvas_t canvas;
    fd_ux_screen_t *screen;
    bool animation;
    bool update_enabled;
    uint32_t idle_ticks;
    fd_ux_state_t state;
} fd_ux_t;

fd_ux_t *fd_ux_get(uint32_t identifier);

void fd_ux_initialize(fd_ux_t *ux, const fd_ux_configuration_t *configuration);
void fd_ux_tick(void);

void fd_ux_add_listener(fd_ux_t *ux, const fd_ux_listener_t *listener);

bool fd_ux_get_update_enabled(fd_ux_t *ux);
void fd_ux_set_update_enabled(fd_ux_t *ux, bool enable);
void fd_ux_update(fd_ux_t *ux);

void fd_ux_active(fd_ux_t *ux);
fd_ux_state_t fd_ux_get_state(fd_ux_t *ux);
void fd_ux_set_state(fd_ux_t *ux, fd_ux_state_t state);

void fd_ux_set_animation(fd_ux_t *ux, bool animation);

fd_ux_screen_t *fd_ux_get_active_screen(fd_ux_t *ux);
uint32_t fd_ux_get_screen(fd_ux_t *ux);
void fd_ux_set_screen(fd_ux_t *ux, uint32_t id);
void fd_ux_set_screen_preview(fd_ux_t *ux, uint32_t id);

void fd_ux_get_display_configuration(fd_ux_t *ux, firefly_ux_v1_DisplayConfiguration *display_configuration);
void fd_ux_set_display_configuration(fd_ux_t *ux, const firefly_ux_v1_DisplayConfiguration *display_configuration);

void fd_ux_get_interaction_configuration(fd_ux_t *ux, firefly_ux_v1_InteractionConfiguration *interaction_configuration);
void fd_ux_set_interaction_configuration(fd_ux_t *ux, const firefly_ux_v1_InteractionConfiguration *interaction_configuration);

typedef enum {
    fd_ux_frame_buffer_type_gray = 0,
    fd_ux_frame_buffer_type_color = 1,
} fd_ux_frame_buffer_type_t;

typedef struct {
    uint32_t depth;
} fd_ux_frame_buffer_gray_t;

typedef struct {
    uint32_t r;
    uint32_t g;
    uint32_t b;
} fd_ux_frame_buffer_color_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    fd_ux_frame_buffer_type_t type;
    union {
        fd_ux_frame_buffer_gray_t gray;
        fd_ux_frame_buffer_color_t color;
    } components;
} fd_ux_frame_buffer_metadata_t;

fd_ux_frame_buffer_metadata_t fd_ux_get_frame_buffer_metadata(fd_ux_t *ux);

typedef struct {
    uint8_t *data;
    size_t size;
} fd_ux_frame_buffer_t;

fd_ux_frame_buffer_t fd_ux_get_frame_buffer(fd_ux_t *ux);

#endif
