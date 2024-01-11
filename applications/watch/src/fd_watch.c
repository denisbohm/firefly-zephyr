#include "fd_watch.h"

#include "fd_watch_screen_powering_off.h"
#include "fd_watch_screen_splash.h"
#include "fd_watch_screen_time.h"

#include "fd_event.h"
#include "fd_font_b612_regular_6.h"
#include "fd_font_b612_regular_12.h"
#include "fd_gpio.h"
#include "fd_graphics_color_16bit.h"
#include "fd_rpc.h"
#include "fd_rpc_channel_usb.h"
#include "fd_rpc_server_rtc.h"
#include "fd_rtc.h"
#include "fd_ssd1331.h"
#include "fd_timer.h"
#include "fd_unused.h"
#include "fd_ux.h"
#include "fd_ux_button.h"

#define fd_watch_display_width 96
#define fd_watch_display_height 64

typedef struct {
    fd_graphics_t graphics;
    fd_graphics_color_16bit_t color_16bit;
    uint8_t frame_buffer[fd_graphics_color_16bit_frame_buffer_size(fd_watch_display_width, fd_watch_display_height)];

    struct k_work tick_work;
    struct k_timer tick_timer;
} fd_watch_t;

fd_watch_t fd_watch;

void fd_watch_ssd1331_update(fd_graphics_t *graphics fd_unused, fd_graphics_area_t area) {
    fd_ssd1331_write_image_start(area.x, area.y, area.width, area.height);
    for (int y = 0; y < area.height; ++y) {
        uint8_t *data = &fd_watch.frame_buffer[((area.y + y) * fd_watch_display_width + area.x) * 2];
        fd_ssd1331_write_image_subdata(data, area.width * 2);
    }
    fd_ssd1331_write_image_end();
}

void fd_watch_ssd1331_on(fd_graphics_t *graphics fd_unused) {
    fd_ssd1331_display_on();
}

void fd_watch_ssd1331_off(fd_graphics_t *graphics fd_unused) {
    fd_ssd1331_display_off();
}

void fd_watch_display_on(void) {
    fd_graphics_display_on(&fd_watch.graphics);
}

void fd_watch_display_off(void) {
    fd_graphics_display_off(&fd_watch.graphics);
}

void fd_watch_display_initialize(void) {
    fd_graphics_t *graphics = &fd_watch.graphics;
    fd_graphics_color_16bit_initialize(
        &fd_watch.color_16bit,
        fd_watch.frame_buffer,
        sizeof(fd_watch.frame_buffer),
        graphics,
        fd_watch_display_width,
        fd_watch_display_height
    );
    fd_ssd1331_initialize();
    fd_watch.graphics.backend.update = fd_watch_ssd1331_update;
    fd_watch.graphics.backend.display_on = fd_watch_ssd1331_on;
    fd_watch.graphics.backend.display_off = fd_watch_ssd1331_off;

    fd_graphics_area_t area = { .x = 0, .y = 0, .width = graphics->width, .height = graphics->height };
    fd_graphics_write_background(graphics);
    fd_graphics_update(graphics, area);

    fd_graphics_set_font(graphics, &fd_font_b612_regular_12);
    fd_graphics_set_foreground(graphics, (fd_graphics_color_t) { .argb=0xff00ff00 });
    fd_graphics_write_string(graphics, 0, 18, "Watch");
    fd_graphics_set_font(graphics, &fd_font_b612_regular_6);
    fd_graphics_set_foreground(graphics, (fd_graphics_color_t) { .argb=0xffff0000 });
    fd_graphics_write_string(graphics, 0, 32, "12:00:00");
    fd_graphics_update(graphics, area);
}

void fd_watch_rpc_initialize(void) {
    fd_rpc_initialize();
    fd_rpc_server_rtc_initialize();

    const fd_rpc_channel_usb_configuration_t usb_configuration = {
        .work_queue = &k_sys_work_q,
    };
    fd_rpc_channel_usb_initialize(&usb_configuration);
}

void fd_watch_power_on(void) {
}

void fd_watch_power_off(void) {
}

void fd_watch_button_event(const fd_ux_button_event_t *event) {
    fd_ux_button_event(event);
}

void fd_watch_tick_work(struct k_work *work fd_unused) {
    fd_timer_update(0.020f);
    fd_ux_tick();
}

void fd_watch_tick_timer_irq(struct k_timer *timer fd_unused) {
    k_work_submit_to_queue(&k_sys_work_q, &fd_watch.tick_work);
}

void fd_watch_initialize(void) {
    fd_gpio_initialize();
    fd_event_initialize();
    fd_rtc_initialize();
    fd_timer_initialize();

    fd_watch_display_initialize();
    
    static fd_ux_screen_t screens[fd_watch_screen_id_count] = {
    };
    fd_watch_screen_splash_initialize(&screens[fd_watch_screen_id_splash]);
    fd_watch_screen_time_initialize(&screens[fd_watch_screen_id_time]);
    fd_watch_screen_powering_off_initialize(&screens[fd_watch_screen_id_powering_off]);
    static fd_ux_configuration_t ux_configuration;
    ux_configuration = (fd_ux_configuration_t) {
        .graphics = &fd_watch.graphics,
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

    fd_watch_rpc_initialize();

    k_work_init(&fd_watch.tick_work, fd_watch_tick_work);
    k_timer_init(&fd_watch.tick_timer, fd_watch_tick_timer_irq, NULL);
    k_timer_start(&fd_watch.tick_timer, K_SECONDS(1), K_MSEC(20));

#if 1
    while (true) {
        k_sleep(K_SECONDS(1));
    }
#endif
}
