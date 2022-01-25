#include <hal/nrf_gpio.h>

void busy_wait(uint32_t ms) {
    for (volatile int i = 0; i < ms; ++i) {
        for (volatile int j = 0; j < 16000; ++j) {
        }
    }
}

void main(void) {
    const uint32_t pin = 31;
    const uint32_t delay = 250;

    nrf_gpio_cfg_output(pin);
    while (true) {
        nrf_gpio_pin_clear(pin);
        busy_wait(delay);
	    nrf_gpio_pin_set(pin);
        busy_wait(delay);
    }
}
