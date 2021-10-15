#ifndef fd_ux_h
#define fd_ux_h

#include "fd_drawing.h"

typedef struct {
    
} fd_button_event_t;

typedef enum {
    fd_ux_screen_id_splash,
    fd_ux_screen_id_watch,
    fd_ux_screen_id_count
} fd_ux_screen_id_t;

typedef struct {
    fd_ux_screen_id_t id;
    fd_drawing_plane_t *planes[10];
    uint32_t plane_count;
    void (*preview)(void);
    void (*activate)(void);
    void (*deactivate)(void);
    bool (*button)(fd_button_event_t event);
    bool (*animate)(void);
} fd_ux_screen_t;

void fd_ux_initialize(void);

void fd_ux_set_animation(bool animation);

void fd_ux_set_screen(fd_ux_screen_id_t id);
void fd_ux_set_screen_preview(fd_ux_screen_id_t id);

#endif
