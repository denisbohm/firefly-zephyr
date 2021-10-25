#include "fd_ssd1327.h"

#include "fd_assert.h"
#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_ssd1327_bus.h"
#include "fd_unused.h"

#include <string.h>

typedef struct {
    fd_ssd1327_configuration_t configuration;
} fd_ssd1327_t;

fd_ssd1327_t fd_ssd1327;

#define fd_SSD1327_SET_COLUMN_ADDRESS 0x15
#define fd_SSD1327_SET_ROW_ADDRESS    0x75
#define fd_SSD1327_SET_DISPLAY_OFF    0xAE
#define fd_SSD1327_SET_DISPLAY_ON     0xAF

static void fd_ssd1327_cs_enable(void) {
    fd_gpio_set(fd_ssd1327.configuration.csx, false);
}

static void fd_ssd1327_cs_disable(void) {
    fd_gpio_set(fd_ssd1327.configuration.csx, true);
}

static void fd_ssd1327_command_mode(void) {
    fd_gpio_set(fd_ssd1327.configuration.dcx, false);
}

static void fd_ssd1327_data_mode(void) {
    fd_gpio_set(fd_ssd1327.configuration.dcx, true);
}

static void fd_ssd1327_reset(void) {
    fd_gpio_set(fd_ssd1327.configuration.resx, true);
    fd_delay_us(100);
    fd_gpio_set(fd_ssd1327.configuration.resx, false);
    fd_delay_us(100);
    fd_gpio_set(fd_ssd1327.configuration.resx, true);
}

static void fd_ssd1327_write_command(const uint8_t *data, uint32_t length) {
    fd_ssd1327_command_mode();
    fd_ssd1327_bus_write(data, length);
}

static void fd_ssd1327_write_command_byte(uint8_t command) {
    fd_ssd1327_command_mode();
    fd_ssd1327_bus_write(&command, 1);
}

static void fd_ssd1327_set_column_address(int start, int count) {
    fd_assert((start & 1) == 0);
    fd_assert(count > 0);
    fd_assert((count & 1) == 0);

    int s = start / 2;
    int e = (start + count) / 2 - 1;
    uint8_t data[] = { fd_SSD1327_SET_COLUMN_ADDRESS, (uint8_t)s, (uint8_t)e };
    fd_ssd1327_write_command(data, sizeof(data));
}

static void fd_ssd1327_set_row_address(int start, int count) {
    fd_assert(count > 0);

    int s = start;
    int e = start + count - 1;
    fd_assert(count != 0);
    uint8_t data[] = { fd_SSD1327_SET_ROW_ADDRESS, (uint8_t)s, (uint8_t)e };
    fd_ssd1327_write_command(data, sizeof(data));
}

void fd_ssd1327_send_commands(const uint8_t *data, size_t length) {
    fd_ssd1327_cs_enable();
    fd_ssd1327_command_mode();
    fd_ssd1327_bus_write(data, (uint32_t)length);
    fd_ssd1327_cs_disable();
}

static void fd_ssd1327_send_init_sequence(void) {
    fd_ssd1327_reset();

    uint8_t data[] = {
        0xfd, 0x12,       // unlock 
        0xae,             // display off
        0x15, 0x00, 0x7f, // set column address, start column 0, end column 127
        0x75, 0x00, 0x7f, // set row address, start row 0, end row 127
        0x81, 0x80,       // set contrast control
        0xa0, 0x51,       // set remap
        0xa1, 0x00,       // start line
        0xa2, 0x00,       // display offset
        0xa4,             // normal display, 
        0xa8, 0x7f,       // set multiplex ratio
        0xb1, 0xf1,       // set phase length
        0xb3, 0x00,       // set dclk 100Hz
        0xab, 0x01,       // function selection a: enable vdd regulator
        0xb6, 0x0f,       // set second precharge period
        0xbe, 0x0f,       // set VCOMH voltage
        0xbc, 0x08,       // set precharge voltage
        0xd5, 0x62,       // function selection b
    };
    fd_ssd1327_send_commands(data, sizeof(data));
    fd_delay_ms(100);
}

void fd_ssd1327_display_on(void) {
    fd_ssd1327_cs_enable();
    fd_ssd1327_write_command_byte(fd_SSD1327_SET_DISPLAY_ON);
    fd_ssd1327_cs_disable();
}

void fd_ssd1327_display_off(void) {
    fd_ssd1327_cs_enable();
    fd_ssd1327_write_command_byte(fd_SSD1327_SET_DISPLAY_OFF);
    fd_ssd1327_cs_disable();
}

void fd_ssd1327_initialize(fd_ssd1327_configuration_t configuration) {
    memset(&fd_ssd1327, 0, sizeof(fd_ssd1327));
    fd_ssd1327.configuration = configuration;

    fd_gpio_configure_output(fd_ssd1327.configuration.resx);
    fd_gpio_set(fd_ssd1327.configuration.resx, true);
    fd_gpio_configure_output(fd_ssd1327.configuration.csx);
    fd_gpio_set(fd_ssd1327.configuration.csx, true);
    fd_gpio_configure_output(fd_ssd1327.configuration.dcx);
    fd_gpio_set(fd_ssd1327.configuration.dcx, true);

    fd_ssd1327_bus_initialize();

    fd_ssd1327_send_init_sequence();
}

void fd_ssd1327_write_image_start(int x, int y, int width, int height) {
    fd_assert(x >= 0);
    fd_assert(y >= 0);
    fd_assert(width > 0);
    fd_assert(height > 0);

    fd_ssd1327_cs_enable();

    fd_ssd1327_set_column_address(x, width);
    fd_ssd1327_set_row_address(y, height);
    fd_ssd1327_data_mode();
}

void fd_ssd1327_write_image_subdata(const uint8_t *data, int length) {
    fd_ssd1327_bus_write(data, (uint32_t)length);
}

void fd_ssd1327_write_image_end(void) {
    fd_ssd1327_cs_disable();
}
