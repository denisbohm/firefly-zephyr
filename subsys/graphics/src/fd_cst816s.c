#include "fd_cst816s.h"

#include "fd_assert.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <stdbool.h>

fd_source_push()

typedef struct {
    fd_cst816s_configuration_t configuration;
    struct k_work work;
    const struct device *device;
    struct gpio_dt_spec reset;
    struct gpio_dt_spec interrupt;
    struct gpio_callback interrupt_callback;
} fd_cst816s_t;

fd_cst816s_t fd_cst816s;

#define fd_cst816s_i2c_address 0x15

/* CST816S will respond with NACK to address when in low-power mode. Host
* needs to retry address selection multiple times to get CST816S to
* wake-up, or reset part.
*/
void fd_cst816s_reset(void) {
    gpio_pin_set_dt(&fd_cst816s.reset, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&fd_cst816s.reset, 0);
    k_sleep(K_MSEC(50));
}

bool fd_cst816s_i2c_read(uint8_t address, uint8_t *data, size_t size) {
    int result = i2c_write_read(fd_cst816s.device, fd_cst816s_i2c_address, &address, sizeof(address), data, size);
    fd_assert(result == 0);
    return result == 0;
}

bool fd_cst816s_read_touch(fd_cst816s_touch_t *touch) {
    uint8_t data[6];
    if (!fd_cst816s_i2c_read(0x01, data, sizeof(data))) {
        return false;
    }
    touch->gesture = data[0];
    touch->finger = data[1];
    touch->x = (data[2] & 0x0f) << 8 | data[3];
    touch->y = (data[4] & 0x0f) << 8 | data[5];
    return true;
}

void fd_cst816s_work(struct k_work *work) {
    fd_cst816s_touch_t touch;
    if (!fd_cst816s_read_touch(&touch)) {
        return;
    }
    fd_cst816s.configuration.touch(&touch);
}

void fd_cst816s_isr(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    k_work_submit_to_queue(fd_cst816s.configuration.work_queue, &fd_cst816s.work);
}

void fd_cst816s_initialize(const fd_cst816s_configuration_t *configuration) {
    fd_cst816s.configuration = *configuration;

    k_work_init(&fd_cst816s.work, fd_cst816s_work);

#if DT_NODE_EXISTS(DT_CHOSEN(firefly_cst816s_i2c))
    fd_cst816s.device = DEVICE_DT_GET(DT_CHOSEN(firefly_cst816s_i2c));
#endif

#if DT_NODE_EXISTS(DT_CHOSEN(firefly_cst816s_rst))
    {
    fd_cst816s.reset = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_cst816s_rst), gpios);
    int result = gpio_pin_configure_dt(&fd_cst816s.reset, GPIO_OUTPUT_INACTIVE);
    fd_assert(result == 0);
    }
#endif

#if DT_NODE_EXISTS(DT_CHOSEN(firefly_cst816s_int))
    {
    fd_cst816s.interrupt = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_cst816s_int), gpios);
    int result = gpio_pin_configure_dt(&fd_cst816s.interrupt, GPIO_INPUT | GPIO_PULL_UP);
    fd_assert(result == 0);
    result = gpio_pin_interrupt_configure_dt(&fd_cst816s.interrupt, GPIO_INT_EDGE_FALLING);
    fd_assert(result == 0);
    gpio_init_callback(&fd_cst816s.interrupt_callback, fd_cst816s_isr, BIT(fd_cst816s.interrupt.pin));
    result = gpio_add_callback(fd_cst816s.interrupt.port, &fd_cst816s.interrupt_callback);
    fd_assert(result == 0);
    }
#endif

    fd_cst816s_reset();

    uint8_t chipid;
    fd_cst816s_i2c_read(0xA7, &chipid, sizeof(chipid));
    fd_assert(chipid == 0xB5);
    uint8_t projid;
    fd_cst816s_i2c_read(0xA8, &projid, sizeof(projid));
    fd_assert(projid == 0x00);
    uint8_t fwversion;
    fd_cst816s_i2c_read(0xA9, &fwversion, sizeof(fwversion));
    fd_assert(fwversion == 0x01);
}

fd_source_pop()