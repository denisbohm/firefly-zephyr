#include "fd_ili9341.h"

#include "fd_assert.h"
#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_ili9341_bus.h"
#include "fd_unused.h"

fd_gpio_t fd_ili9341_gpio_resx;
fd_gpio_t fd_ili9341_gpio_csx;
fd_gpio_t fd_ili9341_gpio_dcx;

#define fd_ILI9341_NOP     0x00
#define fd_ILI9341_SWRESET 0x01
#define fd_ILI9341_RDDID   0x04
#define fd_ILI9341_RDDST   0x09
#define fd_ILI9341_SLPIN   0x10
#define fd_ILI9341_SLPOUT  0x11
#define fd_ILI9341_DISPOFF 0x28
#define fd_ILI9341_DISPON  0x29
#define fd_ILI9341_CASET   0x2A
#define fd_ILI9341_PASET   0x2B
#define fd_ILI9341_RAMWR   0x2C
#define fd_ILI9341_RAMRD   0x2E
#define fd_ILI9341_TEOFF   0x34
#define fd_ILI9341_TEON    0x35
#define fd_ILI9341_MADCTL  0x36
#define fd_ILI9341_PIXSET  0x3A
#define fd_ILI9341_STE     0x44

#define fd_ILI9341_PIXSET_RGB565 0x05

static void fd_ili9341_cs_enable(void) {
    fd_gpio_set(fd_ili9341_gpio_csx, false);
}

static void fd_ili9341_cs_disable(void) {
    fd_gpio_set(fd_ili9341_gpio_csx, true);
}

static void fd_ili9341_command_mode(void) {
    fd_gpio_set(fd_ili9341_gpio_dcx, false);
}

static void fd_ili9341_data_mode(void) {
    fd_gpio_set(fd_ili9341_gpio_dcx, true);
}

static void fd_ili9341_reset(void) {
    fd_gpio_set(fd_ili9341_gpio_resx, true);
    fd_delay_ms(5);
    fd_gpio_set(fd_ili9341_gpio_resx, false);
    fd_delay_ms(20);
    fd_gpio_set(fd_ili9341_gpio_resx, true);
    fd_delay_ms(180);
}

#ifdef fd_ILI9341_CAN_READ

static void fd_ili9341_read_data(uint8_t *data, uint32_t length) {
    fd_ili9341_data_mode();
    fd_ili9341_bus_read(data, length);
}

#endif

static void fd_ili9341_write_data(const uint8_t *data, uint32_t length) {
    fd_ili9341_data_mode();
    fd_ili9341_bus_write(data, length);
}

static void fd_ili9341_write_data_byte(uint8_t data) {
    fd_ili9341_write_data(&data, 1);
}

static void fd_ili9341_write_command_byte(uint8_t command) {
    fd_ili9341_command_mode();
    fd_ili9341_bus_write(&command, 1);
}

static void fd_ili9341_paset(int s, int e) {
    uint8_t data[] = {
        (uint8_t)(s >> 8), (uint8_t)s,
        (uint8_t)(e >> 8), (uint8_t)e,
    };
    fd_ili9341_write_command_byte(fd_ILI9341_PASET);
    fd_ili9341_write_data(data, sizeof(data));
}

static void fd_ili9341_caset(int s, int e) {
    uint8_t data[] = {
        (uint8_t)(s >> 8), (uint8_t)s,
        (uint8_t)(e >> 8), (uint8_t)e,
    };
    fd_ili9341_write_command_byte(fd_ILI9341_CASET);
    fd_ili9341_write_data(data, sizeof(data));
}

#ifdef fd_ILI9341_CAN_READ

static uint8_t fd_ili9341_id[4];

#endif

static void fd_ili9341_send_init_sequence(void) {
    fd_ili9341_reset();

    fd_ili9341_cs_enable();

#ifdef fd_ILI9341_CAN_READ
    fd_ili9341_write_command_byte(fd_ILI9341_RDDID);
    fd_ili9341_read_data(fd_ili9341_id, sizeof(fd_ili9341_id));
    fd_assert(fd_ili9341_id[1] == 0x85);
    fd_assert(fd_ili9341_id[2] == 0x85);
    fd_assert(fd_ili9341_id[3] == 0x52);
#endif

    fd_ili9341_write_command_byte(fd_ILI9341_SLPOUT);
    fd_delay_ms(5);

    fd_ili9341_write_command_byte(fd_ILI9341_PIXSET);
    fd_ili9341_write_data_byte(fd_ILI9341_PIXSET_RGB565);

    fd_ili9341_caset(0, 240);
    fd_ili9341_paset(0, 320);

    fd_ili9341_cs_disable();
}

void fd_ili9341_display_on(void) {
    fd_ili9341_cs_enable();
    fd_ili9341_write_command_byte(fd_ILI9341_DISPON);
    fd_ili9341_cs_disable();
}

void fd_ili9341_display_off(void) {
    fd_ili9341_cs_enable();
    fd_ili9341_write_command_byte(fd_ILI9341_DISPOFF);
    fd_ili9341_cs_disable();
}

void fd_ili9341_initialize(void) {
    fd_ili9341_gpio_resx = (fd_gpio_t) { .port = 1, .pin = 1 };
    fd_ili9341_gpio_csx = (fd_gpio_t) { .port = 1, .pin = 4 };
    fd_ili9341_gpio_dcx = (fd_gpio_t) { .port = 1, .pin = 5 };

    fd_gpio_configure_output(fd_ili9341_gpio_resx, true);
    fd_gpio_configure_output(fd_ili9341_gpio_csx, true);
    fd_gpio_configure_output(fd_ili9341_gpio_dcx, true);

    fd_ili9341_bus_initialize();

    fd_ili9341_send_init_sequence();
}

void fd_ili9341_write_image_start(int x, int y, int width, int height) {
    fd_ili9341_cs_enable();

    fd_ili9341_paset(x, x + width - 1);
    fd_ili9341_caset(y, y + height - 1);
    fd_ili9341_write_command_byte(fd_ILI9341_RAMWR);
    fd_ili9341_data_mode();
}

void fd_ili9341_write_image_subdata(const uint8_t *data, int length) {
    fd_ili9341_bus_write_pixel(data, (uint32_t)length);
}

void fd_ili9341_write_image_end(void) {
    fd_ili9341_cs_disable();
}
