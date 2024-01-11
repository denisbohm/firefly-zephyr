#include "fd_ssd1331.h"

#include "fd_assert.h"
#include "fd_spi_display.h"

#define FD_SSD1331_WIDTH 96
#define FD_SSD1331_HEIGHT 64
#define FD_SSD1331_X_SHIFT 0
#define FD_SSD1331_Y_SHIFT 0

void fd_ssd1331_initialize(void) {
    fd_spi_display_initialize();
    fd_spi_display_reset();

    uint8_t commands[] = {
        fd_spi_display_command(0xAE), // display off
        fd_spi_display_command(0xA0, 0x72), // set remap, RGB color, 65k color format
        fd_spi_display_command(0xA1, 0x00), // start line
        fd_spi_display_command(0xA2, 0x00), // display offset
        fd_spi_display_command(0xA4), // normal display
        fd_spi_display_command(0xA8, 0x3F), // set multiplex 1/64 duty
        fd_spi_display_command(0xAD, 0x8E), // set master
        fd_spi_display_command(0xB0, 0x0B), // power mode
        fd_spi_display_command(0xB1, 0x31), // precharge
        fd_spi_display_command(0xB3, 0xF0), // clock div, 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio, (A[3:0]+1 = 1..16)
        fd_spi_display_command(0x8A, 0x64), // precharge A
        fd_spi_display_command(0x8B, 0x78), // precharge B
        fd_spi_display_command(0x8C, 0x64), // precharge C
        fd_spi_display_command(0xBB, 0x3A), // precharge level
        fd_spi_display_command(0xBE, 0x3E), // vcomh
        fd_spi_display_command(0x87, 0x06), // master current
        fd_spi_display_command(0x81, 0x91), // contrast A
        fd_spi_display_command(0x82, 0x50), // contrast B
        fd_spi_display_command(0x83, 0x7D), // contrast C

        fd_spi_display_command(0xAF), // display on
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}

void fd_ssd1331_display_on(void) {
    uint8_t commands[] = {
        fd_spi_display_command(0xAF), // display on
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}

void fd_ssd1331_display_off(void) {
    uint8_t commands[] = {
        fd_spi_display_command(0xAE), // display off
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}

void fd_ssd1331_write_image_start(int x, int y, int width, int height) {
    fd_assert(x >= 0);
    fd_assert((x + width) <= FD_SSD1331_WIDTH);
    fd_assert(y >= 0);
    fd_assert((y + height) <= FD_SSD1331_HEIGHT);
    x += FD_SSD1331_X_SHIFT;
    y += FD_SSD1331_Y_SHIFT;
    const int x1 = x + width - 1;
    const int y1 = y + height - 1;
    uint8_t commands[] = {
        fd_spi_display_command(0x15, x, x1), // set column start and end address
        fd_spi_display_command(0x75, y, y1), // set row start and end address
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
}

void fd_ssd1331_write_image_subdata(const uint8_t *data, size_t size) {
    fd_spi_display_write_data(data, size);
}

void fd_ssd1331_write_image_end(void) {
    uint8_t commands[] = {
    };
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}
