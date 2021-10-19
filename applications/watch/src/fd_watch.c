#include "fd_watch.h"

#include "fd_watch_screen_powering_off.h"
#include "fd_watch_screen_splash.h"
#include "fd_watch_screen_time.h"

#include "fd_ux.h"
#include "fd_ux_button.h"

#include "fd_graphics_ssd1327.h"
#include "fd_ssd1327.h"

#include "fd_event.h"
#include "fd_gpio.h"
#include "fd_timer.h"

void fd_watch_power_on(void) {
    fd_ssd1327_display_on();
    fd_ux_power_on();
}

void fd_watch_power_off(void) {
    fd_ssd1327_display_off();
    fd_ux_power_off();
}

void fd_watch_button_event(const fd_ux_button_event_t *event) {
    if (!fd_ux_is_powered_on()) {
        fd_watch_power_on();
        fd_ux_set_screen(fd_watch_screen_id_time);
    } else
    if (fd_ux_button_is_all_activated(event, fd_watch_button_both)) {
        fd_ux_set_screen(fd_watch_screen_id_powering_off);
    } else {
        fd_ux_button_event(event);
    }
}

void fd_watch_initialize(void) {
    fd_gpio_initialize();
    fd_event_initialize();
    fd_timer_initialize();
    fd_graphics_ssd1327_initialize();
    
    static fd_ux_screen_t screens[fd_watch_screen_id_count] = {
    };
    fd_watch_screen_splash_initialize(&screens[fd_watch_screen_id_splash]);
    fd_watch_screen_time_initialize(&screens[fd_watch_screen_id_time]);
    fd_watch_screen_powering_off_initialize(&screens[fd_watch_screen_id_powering_off]);
    static fd_ux_configuration_t ux_configuration;
    ux_configuration = (fd_ux_configuration_t) {
        .graphics = fd_graphics_ssd1327_get(),
        .screens = screens,
        .screen_count = sizeof(screens) / sizeof(screens[0]),
    };
    fd_ux_initialize(&ux_configuration);

    static fd_gpio_t button_gpios[] = {
        { .port = 0, .pin = 9 },
        { .port = 1, .pin = 10 },
    };
    static fd_ux_button_configuration_t button_configuration = {
        .gpios = button_gpios,
        .count = sizeof(button_gpios) / sizeof(button_gpios[0]),
        .callback = fd_watch_button_event,
    };
    fd_ux_button_initialize(&button_configuration);
}
