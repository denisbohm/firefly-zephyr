#ifndef fd_watch_h
#define fd_watch_h

#define fd_watch_button_left  (1 << 0)
#define fd_watch_button_right (1 << 1)
#define fd_watch_button_both (fd_watch_button_left | fd_watch_button_right)

typedef enum {
    fd_watch_screen_id_splash,
    fd_watch_screen_id_time,
    fd_watch_screen_id_powering_off,
    fd_watch_screen_id_count
} fd_watch_screen_id_t;

void fd_watch_initialize(void);

void fd_watch_power_on(void);
void fd_watch_power_off(void);

#endif
