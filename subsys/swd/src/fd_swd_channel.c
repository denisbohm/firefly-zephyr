#include "fd_swd_channel.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/posix/time.h>

#if 0
#include <soc.h>
#endif

fd_swd_channel_t fd_swd_channels[fd_swd_channel_count];

fd_swd_channel_t *fd_swd_channel_get(uint32_t index) {
    return &fd_swd_channels[index];
}

void fd_swd_channel_set_power(fd_swd_channel_t *channel, bool value) {
    if (channel->gpio_power.port != NULL) {
        gpio_pin_set_dt(&channel->gpio_power, value ? 1 : 0);
    }
}

void fd_swd_channel_set_reset(fd_swd_channel_t *channel, bool value) {
    gpio_pin_set_dt(&channel->gpio_nreset, value ? 1 : 0);
}

#define fd_swd_channel_gpio_on(gpio) gpio_pin_set_dt(&gpio, 1)
#define fd_swd_channel_gpio_off(gpio) gpio_pin_set_dt(&gpio, 0)
#define fd_swd_channel_gpio_get(gpio) (gpio_pin_get_dt(&gpio) == 1)
#define fd_swd_channel_gpio_set_mode_input(gpio) gpio_pin_configure(gpio.port, gpio.pin, GPIO_INPUT)
#define fd_swd_channel_gpio_set_mode_output(gpio) gpio_pin_configure(gpio.port, gpio.pin, GPIO_OUTPUT)

// SWDCLK has a pull down resistor
// SWDIO has a pull up resistor
// SWD Master writes and reads data on falling clock edge

void fd_swd_channel_debug_out(struct gpio_dt_spec clock, struct gpio_dt_spec data, uint8_t *out, uint32_t length) {
#if 0
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    fd_log_assert(data_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *data_address = &data_gpio->port->BSRR;
    uint32_t data_on_bit = 0x00000001 << data_gpio->pin;
    uint32_t data_off_bit = 0x00010000 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = *out++;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_address = clock_on_bit;
//            fd_swd_channel_gpio_on(channel->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            if (byte & 1) {
                *data_address = data_on_bit;
//                fd_swd_channel_gpio_on(channel->gpio_data);
            } else {
                *data_address = data_off_bit;
//                fd_swd_channel_gpio_off(channel->gpio_data);
            }
            byte >>= 1;
        
            *clock_address = clock_off_bit;
//            fd_swd_channel_gpio_off(channel->gpio_clock);
        }
    }
#endif
}   

void fd_swd_channel_debug_in(struct gpio_dt_spec clock, struct gpio_dt_spec data, uint8_t *in, uint32_t length) {
#if 0
    fdi_gpio_t *clock_gpio = &fdi_gpios[clock];
    fd_log_assert(clock_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *clock_address = &clock_gpio->port->BSRR;
    uint32_t clock_on_bit = 0x00000001 << clock_gpio->pin;
    uint32_t clock_off_bit = 0x00010000 << clock_gpio->pin;

    fdi_gpio_t *data_gpio = &fdi_gpios[data];
    fd_log_assert(data_gpio->status == fdi_gpio_status_valid);
    __IO uint32_t *data_address = &data_gpio->port->IDR;
    uint32_t data_bit = 1 << data_gpio->pin;

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; ++j) {
//            fdi_serial_wire_half_bit_delay();
            *clock_address = clock_on_bit;
//            fd_swd_channel_gpio_on(channel->gpio_clock);
//            fdi_serial_wire_half_bit_delay();

            byte >>= 1;
            if (*data_address & data_bit) {
                byte |= 0b10000000;
            }

            *clock_address = clock_off_bit;
//            fd_swd_channel_gpio_off(channel->gpio_clock);
        }
        *in++ = byte;
    }
#endif
}

void fd_swd_channel_delay_ns(uint32_t delay_ns) {
    uint32_t delay_cycles = ((sys_clock_hw_cycles_per_sec() >> 10) * delay_ns) / (1000000000 >> 10);
    int start = k_cycle_get_32();
    while (true) {
        int now = k_cycle_get_32();
        int duration = now - start;
        if (duration >= delay_cycles) {
            break;
        }
    }
}

#define fd_swd_channel_half_bit_delay() {\
    int delay_ns = channel->half_bit_delay_ns;\
    if (delay_ns > 0) {\
        fd_swd_channel_delay_ns(delay_ns);\
    }\
}

void fd_swd_channel_shift_out(fd_swd_channel_t *channel, uint8_t byte, int bit_count) {
    while (bit_count-- > 0) {
        fd_swd_channel_half_bit_delay();
        fd_swd_channel_gpio_on(channel->gpio_clock);
        fd_swd_channel_half_bit_delay();

        if (byte & 1) {
            fd_swd_channel_gpio_on(channel->gpio_data);
        } else {
            fd_swd_channel_gpio_off(channel->gpio_data);
        }
        byte >>= 1;

        fd_swd_channel_gpio_off(channel->gpio_clock);
    }
}

void fd_swd_channel_shift_out_bytes(fd_swd_channel_t *channel, uint8_t *data, uint32_t length) {
    if (channel->half_bit_delay_ns > 0) {
        for (uint32_t i = 0; i < length; ++i) {
            fd_swd_channel_shift_out(channel, data[i], 8);
        }
    } else {
        fd_swd_channel_debug_out(channel->gpio_clock, channel->gpio_data, data, length);
    }
}

uint8_t fd_swd_channel_shift_in(fd_swd_channel_t *channel, int bit_count) {
    uint8_t byte = 0;
    while (bit_count-- > 0) {
        fd_swd_channel_half_bit_delay();
        fd_swd_channel_gpio_on(channel->gpio_clock);
        fd_swd_channel_half_bit_delay();

        byte >>= 1;
        if (fd_swd_channel_gpio_get(channel->gpio_data)) {
            byte |= 0b10000000;
        }

        fd_swd_channel_gpio_off(channel->gpio_clock);
    }
    return byte;
}

void fd_swd_channel_shift_in_bytes(fd_swd_channel_t *channel, uint8_t *data, uint32_t length) {
    if (channel->half_bit_delay_ns > 0) {
        for (uint32_t i = 0; i < length; ++i) {
            data[i] = fd_swd_channel_shift_in(channel, 8);
        }
    } else {
        fd_swd_channel_debug_in(channel->gpio_clock, channel->gpio_data, data, length);
    }
}

void fd_swd_channel_set_direction_to_read(fd_swd_channel_t *channel) {
    fd_swd_channel_gpio_set_mode_input(channel->gpio_data);
    if (channel->gpio_direction.port != NULL) {
        fd_swd_channel_gpio_off(channel->gpio_direction);
    }
}

void fd_swd_channel_set_direction_to_write(fd_swd_channel_t *channel) {
    fd_swd_channel_gpio_on(channel->gpio_data);
    if (channel->gpio_direction.port != NULL) {
        fd_swd_channel_gpio_on(channel->gpio_direction);
    }
    fd_swd_channel_gpio_set_mode_output(channel->gpio_data);
}

void fd_swd_channel_reinitialize(fd_swd_channel_t *channel) {
    if (channel->gpio_power.port != NULL) {
        fd_swd_channel_gpio_off(channel->gpio_power);
    }
    fd_swd_channel_gpio_set_mode_input(channel->gpio_data);
    if (channel->gpio_direction.port != NULL) {
        fd_swd_channel_gpio_off(channel->gpio_direction);
    }
    fd_swd_channel_gpio_on(channel->gpio_clock);
    fd_swd_channel_gpio_off(channel->gpio_nreset);

    channel->overrun_detection_enabled = false;
    channel->ack_wait_retry_count = 30;
    channel->register_retry_count = 30;
    channel->half_bit_delay_ns = 0;

    channel->target_id = 0;
    channel->debug_port_access.value = 0;
}

void fd_swd_channel_initialize(void) {
    {
        fd_swd_channel_t *channel = &fd_swd_channels[0];
        channel->gpio_nreset = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_nreset), gpios);
        int result = gpio_pin_configure_dt(&channel->gpio_nreset, GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
        fd_assert(result == 0);
        channel->gpio_clock = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_clock), gpios);
        result = gpio_pin_configure_dt(&channel->gpio_clock, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
        fd_assert(result == 0);
        channel->gpio_data = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_data), gpios);
        result = gpio_pin_configure_dt(&channel->gpio_data, GPIO_INPUT);
        fd_assert(result == 0);

#if DT_NODE_EXISTS(DT_CHOSEN(firefly_swd_0_direction))
            channel->gpio_direction = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_direction), gpios);
            int result = gpio_pin_configure_dt(&channel->gpio_direction, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
            fd_assert(result == 0);
#endif
#if DT_NODE_EXISTS(DT_CHOSEN(firefly_swd_0_nreset_sense))
            channel->gpio_nreset_sense = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_nreset_sense), gpios);
            result = gpio_pin_configure_dt(&channel->gpio_nreset_sense, GPIO_INPUT);
            fd_assert(result == 0);
#endif
#if DT_NODE_EXISTS(DT_CHOSEN(firefly_swd_0_power))
            channel->gpio_power = (struct gpio_dt_spec)GPIO_DT_SPEC_GET(DT_CHOSEN(firefly_swd_0_power), gpios);
            result = gpio_pin_configure_dt(&channel->gpio_power, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
            fd_assert(result == 0);
#endif

        channel->overrun_detection_enabled = false;
        channel->ack_wait_retry_count = 30;
        channel->register_retry_count = 30;
        channel->half_bit_delay_ns = 0;
        channel->target_id = 0;
        channel->debug_port_access.value = 0;
    }
}
