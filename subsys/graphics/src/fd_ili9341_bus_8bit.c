#include "fd_ili9341_bus.h"

#include "fd_delay.h"
#include "fd_gpio.h"

// 8080-I series 8-Bit Parallel Interface

#define fd_ili9341_port 1
#define fd_ili9341_wrx_pin 6
#define fd_ili9341_rdx_pin 7
#define fd_ili9341_d0_pin 8

#define fd_ili9341_wrx_mask  (1 << fd_ili9341_wrx_pin)
#define fd_ili9341_rdx_mask  (1 << fd_ili9341_rdx_pin)
#define fd_ili9341_data_mask (0xFF << fd_ili9341_d0_pin)

static fd_gpio_t fd_ili9341_gpio_wrx;
static fd_gpio_t fd_ili9341_gpio_rdx;
static fd_gpio_t fd_ili9341_gpio_data[8];

static void fd_ili9341_configure_data_as_inputs(void) {
    for (uint32_t i = 0; i < 8; ++i) {
        fd_gpio_configure_input(fd_ili9341_gpio_data[i]);
    }
}

static void fd_ili9341_configure_data_as_outputs(void) {
    for (uint32_t i = 0; i < 8; ++i) {
        fd_gpio_configure_output(fd_ili9341_gpio_data[i]);
    }
}

void fd_ili9341_bus_write(const uint8_t *data, uint32_t length) {
    const uint8_t *end = data + length;
    while (data < end) {
        fd_gpio_port_clear_bits(fd_ili9341_port, fd_ili9341_data_mask | fd_ili9341_wrx_mask);
        fd_gpio_port_set_bits(fd_ili9341_port, ((uint32_t)*data++) << fd_ili9341_d0_pin);
        fd_gpio_port_set_bits(fd_ili9341_port, fd_ili9341_wrx_mask);
    }
}

void fd_ili9341_bus_read(uint8_t *data, uint32_t length) {
    fd_ili9341_configure_data_as_inputs();

    const uint8_t *end = data + length;
    while (data < end) {
        fd_gpio_port_clear_bits(fd_ili9341_port, fd_ili9341_rdx_mask);
        *data++ = (uint8_t)(fd_gpio_port_get(fd_ili9341_port) >> fd_ili9341_d0_pin);
        fd_gpio_port_set_bits(fd_ili9341_port, fd_ili9341_rdx_mask);
    }
    fd_delay_us(1);

    fd_ili9341_configure_data_as_outputs();
}

void fd_ili9341_bus_write_pixel(const uint8_t *data, uint32_t length);

// Write 8-bit data for RGB 5-6-5 bits input (65K-Color), 3Ah=05h (DBI[2:0]=101b), IM[3:0]=0000b
/*
D7 R4 G2
D6 R3 G1
D5 R2 G0
D4 R1 B4
D3 R0 B3
D2 G5 B2
D1 G4 B1
D0 G3 B0
*/

void fd_ili9341_bus_write_pixel(const uint8_t *data, uint32_t length) {
    const uint8_t *end = data + length;
    while (data < end) {
        fd_gpio_port_clear_bits(fd_ili9341_port, fd_ili9341_data_mask | fd_ili9341_wrx_mask); // clear d7-0 and wrx
        uint32_t r = *data++;
        fd_gpio_port_set_bits(fd_ili9341_port, (r << fd_ili9341_d0_pin) & (0xF8 << fd_ili9341_d0_pin));
        uint32_t g = *data++;
        fd_gpio_port_set_bits(fd_ili9341_port, (g << (fd_ili9341_d0_pin - 5)) & (0x07 << fd_ili9341_d0_pin));
        fd_gpio_port_set_bits(fd_ili9341_port, fd_ili9341_wrx_mask); // set wrx

        fd_gpio_port_clear_bits(fd_ili9341_port, fd_ili9341_data_mask | fd_ili9341_wrx_mask); // clear d7-0 and wrx
        fd_gpio_port_set_bits(fd_ili9341_port, (g << (fd_ili9341_d0_pin + 3)) & (0xE0 << fd_ili9341_d0_pin));
        uint32_t b = *data++;
        fd_gpio_port_set_bits(fd_ili9341_port, (b << (fd_ili9341_d0_pin - 3)) & (0x1F << fd_ili9341_d0_pin));
        fd_gpio_port_set_bits(fd_ili9341_port, fd_ili9341_wrx_mask); // set wrx
    }
}

void fd_ili9341_bus_initialize(void) {
    fd_ili9341_gpio_wrx = (fd_gpio_t) { .port = fd_ili9341_port, .pin = fd_ili9341_wrx_pin };
    fd_ili9341_gpio_rdx = (fd_gpio_t) { .port = fd_ili9341_port, .pin = fd_ili9341_rdx_pin };
    for (int i = 0; i < 8; ++i) {
        fd_ili9341_gpio_data[i] = (fd_gpio_t) { .port = fd_ili9341_port, .pin = fd_ili9341_d0_pin + i };
    }

    fd_gpio_configure_output(fd_ili9341_gpio_wrx);
    fd_gpio_set(fd_ili9341_gpio_wrx, true);
    fd_gpio_configure_output(fd_ili9341_gpio_rdx);
    fd_gpio_set(fd_ili9341_gpio_rdx, true);
    fd_ili9341_configure_data_as_outputs();
    for (uint32_t i = 0; i < 8; ++i) {
        fd_gpio_set(fd_ili9341_gpio_data[i], false);
    }
}
