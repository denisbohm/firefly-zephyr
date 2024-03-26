#include "fd_gpio.h"

#include "fd_assert.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <stdio.h>

typedef struct {
    const struct device *device;
    fd_gpio_callback_t callbacks[32];
    fd_gpio_listener_t listeners[32];
    struct gpio_callback gpio_callbacks[32];
} fd_gpio_port_metadata_t;

fd_gpio_port_metadata_t fd_gpio_port_metadatas[CONFIG_FIREFLY_SUBSYS_BASE_GPIO_PORT_COUNT];

void fd_gpio_initialize(void) {
    char name[16];
    for (int i = 0; i < CONFIG_FIREFLY_SUBSYS_BASE_GPIO_PORT_COUNT; ++i) {
        fd_gpio_port_metadata_t *metadata = &fd_gpio_port_metadatas[i];
        snprintf(name, sizeof(name), "GPIO_%d", i);
        metadata->device = device_get_binding(name);
        if (metadata->device == NULL) {
            break;
        }
    }
}

static fd_gpio_port_metadata_t *fd_gpio_get_metadata_for_device(const struct device *device) {
    for (int i = 0; i < CONFIG_FIREFLY_SUBSYS_BASE_GPIO_PORT_COUNT; ++i) {
        fd_gpio_port_metadata_t *metadata = &fd_gpio_port_metadatas[i];
        if (metadata->device == device) {
            return metadata;
        }
    }
    return 0;
}

static fd_gpio_port_metadata_t *fd_gpio_get_metadata(int port) {
    fd_assert(port < CONFIG_FIREFLY_SUBSYS_BASE_GPIO_PORT_COUNT);
    return &fd_gpio_port_metadatas[port];
}

static const struct device *fd_gpio_get_device(int port) {
    fd_assert(port < CONFIG_FIREFLY_SUBSYS_BASE_GPIO_PORT_COUNT);
    return fd_gpio_port_metadatas[port].device;
}

void fd_gpio_callback_handler(const struct device *device, struct gpio_callback *gpio_callback, gpio_port_pins_t pins) {
    fd_gpio_port_metadata_t *metadata = fd_gpio_get_metadata_for_device(device);
    for (int i = 0; i < 32; ++i) {
        if (pins & (1 << i)) {
            fd_gpio_callback_t callback = metadata->callbacks[i];
            if (callback != NULL) {
                callback();
            }
            fd_gpio_listener_t *listener = &metadata->listeners[i];
            if (listener->change != NULL) {
                listener->change(listener->context);
            }
        }
    }
}

void fd_gpio_set_callback_common(fd_gpio_t gpio, fd_gpio_edge_t edge) {
    fd_gpio_port_metadata_t *metadata = fd_gpio_get_metadata(gpio.port);
    int flags;
    switch (edge) {
        case fd_gpio_edge_rising:
            flags = GPIO_INT_EDGE_RISING;
            break;
        case fd_gpio_edge_falling:
            flags = GPIO_INT_EDGE_FALLING;
            break;
        case fd_gpio_edge_both:
        default:
            flags = GPIO_INT_EDGE_BOTH;
            break;
    }
    int result = gpio_pin_interrupt_configure(metadata->device, gpio.pin, flags);
    fd_assert(result == 0);
    
    struct gpio_callback *gpio_callback = &metadata->gpio_callbacks[gpio.pin];
    gpio_init_callback(gpio_callback, fd_gpio_callback_handler, 1 << gpio.pin);
    result = gpio_add_callback(metadata->device, gpio_callback);
    fd_assert(result == 0);
}

void fd_gpio_set_callback(fd_gpio_t gpio, fd_gpio_edge_t edge, fd_gpio_callback_t callback) {
    fd_gpio_port_metadata_t *metadata = fd_gpio_get_metadata(gpio.port);
    metadata->callbacks[gpio.pin] = callback;
    fd_gpio_set_callback_common(gpio, edge);
}

void fd_gpio_set_listener(fd_gpio_t gpio, fd_gpio_edge_t edge, fd_gpio_listener_t listener) {
    fd_gpio_port_metadata_t *metadata = fd_gpio_get_metadata(gpio.port);
    metadata->listeners[gpio.pin] = listener;
    fd_gpio_set_callback_common(gpio, edge);
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

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    gpio_pin_configure(fd_gpio_get_device(gpio.port), gpio.pin, GPIO_INPUT | GPIO_PULL_DOWN);
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
