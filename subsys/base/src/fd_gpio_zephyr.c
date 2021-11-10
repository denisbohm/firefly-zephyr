#include "fd_gpio.h"

#include "fd_assert.h"

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include <stdio.h>

typedef struct {
    const struct device *device;
} fd_gpio_port_metadata_t;

#define fd_gpio_port_metadata_count 2

fd_gpio_port_metadata_t fd_gpio_port_metadatas[fd_gpio_port_metadata_count];

void fd_gpio_initialize(void) {
    char name[16];
    for (int i = 0; i < fd_gpio_port_metadata_count; ++i) {
        fd_gpio_port_metadata_t *metadata = &fd_gpio_port_metadatas[i];
        snprintf(name, sizeof(name), "GPIO_%d", i);
        metadata->device = device_get_binding(name);
        fd_assert(metadata->device);
    }
}

static const struct device *fd_gpio_get_device(int port) {
    fd_assert(port < fd_gpio_port_metadata_count);
    return fd_gpio_port_metadatas[port].device;
}

void fd_gpio_set_callback(fd_gpio_t gpio, fd_gpio_callback_t callback) {
}

void fd_gpio_configure_default(fd_gpio_t gpio) {
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, GPIO_DISCONNECTED);
}

void fd_gpio_configure_output(fd_gpio_t gpio, bool value) {
    gpio_flags_t flags = GPIO_OUTPUT;
    if (value) {
        flags |= GPIO_OUTPUT_INIT_HIGH;
    } else {
        flags |= GPIO_OUTPUT_INIT_LOW;
    }
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, flags);
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio, bool value) {
    gpio_flags_t flags = GPIO_INPUT | GPIO_OUTPUT | GPIO_OPEN_DRAIN;
    if (value) {
        flags |= GPIO_OUTPUT_INIT_HIGH;
    } else {
        flags |= GPIO_OUTPUT_INIT_LOW;
    }
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, flags);
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio, bool value) {
    gpio_flags_t flags = GPIO_INPUT | GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP;
    if (value) {
        flags |= GPIO_OUTPUT_INIT_HIGH;
    } else {
        flags |= GPIO_OUTPUT_INIT_LOW;
    }
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, flags);
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, GPIO_INPUT);
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, GPIO_INPUT | GPIO_PULL_UP);
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    gpio_pin_set(fd_gpio_get_device(gpio.port), gpio.pin, value ? 1 : 0);
}

bool fd_gpio_get(fd_gpio_t gpio) {
    return gpio_pin_get(fd_gpio_get_device(gpio.port), gpio.pin) == 1;
}

#if !defined(fd_GPIO_NRF)

void fd_gpio_port_set_bits(int port, uint32_t bits) {
    int result = gpio_port_set_bits(fd_gpio_get_device(port), bits);
    fd_assert(result == 0);
}

void fd_gpio_port_clear_bits(int port, uint32_t bits) {
    int result = gpio_port_clear_bits(fd_gpio_get_device(port), bits);
    fd_assert(result == 0);
}

uint32_t fd_gpio_port_get(int port) {
    uint32_t value = 0;
    int result = gpio_port_get(fd_gpio_get_device(port), &value);
    fd_assert(result == 0);
    return value;
}

#endif
