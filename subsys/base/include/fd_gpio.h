#ifndef fd_gpio_h
#define fd_gpio_h

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int port;
    int pin;
} fd_gpio_t;

void fd_gpio_initialize(void);

void fd_gpio_configure_default(fd_gpio_t gpio);
void fd_gpio_configure_output(fd_gpio_t gpio, bool value);
void fd_gpio_configure_output_open_drain(fd_gpio_t gpio, bool value);
void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio, bool value);
void fd_gpio_configure_input(fd_gpio_t gpio);
void fd_gpio_configure_input_pull_up(fd_gpio_t gpio);
void fd_gpio_configure_input_pull_down(fd_gpio_t gpio);

typedef void (*fd_gpio_callback_t)(void);

typedef enum {
    fd_gpio_edge_rising,
    fd_gpio_edge_falling,
    fd_gpio_edge_both,
} fd_gpio_edge_t;

void fd_gpio_set_callback(fd_gpio_t gpio, fd_gpio_edge_t edge, fd_gpio_callback_t callback);

void fd_gpio_set(fd_gpio_t gpio, bool value);
bool fd_gpio_get(fd_gpio_t gpio);

void fd_gpio_port_set_bits(int port, uint32_t bits);
void fd_gpio_port_clear_bits(int port, uint32_t bits);
uint32_t fd_gpio_port_get(int port);

#endif
