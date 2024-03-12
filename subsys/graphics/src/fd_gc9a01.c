#include "fd_gc9a01.h"

#include "fd_assert.h"
#include "fd_spi_display.h"

#include <zephyr/kernel.h>

#define fd_gc9a01_width 240
#define fd_gc9a01_height 240
#define fd_gc9a01_x_shift 0
#define fd_gc9a01_y_shift 0

#define GC9A01A_SWRESET 0x01   ///< Software Reset (maybe, not documented)
#define GC9A01A_RDDID 0x04     ///< Read display identification information
#define GC9A01A_RDDST 0x09     ///< Read Display Status
#define GC9A01A_SLPIN 0x10     ///< Enter Sleep Mode
#define GC9A01A_SLPOUT 0x11    ///< Sleep Out
#define GC9A01A_PTLON 0x12     ///< Partial Mode ON
#define GC9A01A_NORON 0x13     ///< Normal Display Mode ON
#define GC9A01A_INVOFF 0x20    ///< Display Inversion OFF
#define GC9A01A_INVON 0x21     ///< Display Inversion ON
#define GC9A01A_DISPOFF 0x28   ///< Display OFF
#define GC9A01A_DISPON 0x29    ///< Display ON
#define GC9A01A_CASET 0x2A     ///< Column Address Set
#define GC9A01A_RASET 0x2B     ///< Row Address Set
#define GC9A01A_RAMWR 0x2C     ///< Memory Write
#define GC9A01A_PTLAR 0x30     ///< Partial Area
#define GC9A01A_VSCRDEF 0x33   ///< Vertical Scrolling Definition
#define GC9A01A_TEOFF 0x34     ///< Tearing Effect Line OFF
#define GC9A01A_TEON 0x35      ///< Tearing Effect Line ON
#define GC9A01A_MADCTL 0x36    ///< Memory Access Control
#define GC9A01A_VSCRSADD 0x37  ///< Vertical Scrolling Start Address
#define GC9A01A_IDLEOFF 0x38   ///< Idle mode OFF
#define GC9A01A_IDLEON 0x39    ///< Idle mode ON
#define GC9A01A_COLMOD 0x3A    ///< Pixel Format Set
#define GC9A01A_CONTINUE 0x3C  ///< Write Memory Continue
#define GC9A01A_TEARSET 0x44   ///< Set Tear Scanline
#define GC9A01A_GETLINE 0x45   ///< Get Scanline
#define GC9A01A_SETBRIGHT 0x51 ///< Write Display Brightness
#define GC9A01A_SETCTRL 0x53   ///< Write CTRL Display
#define GC9A01A1_POWER7 0xA7   ///< Power Control 7
#define GC9A01A_TEWC 0xBA      ///< Tearing effect width control
#define GC9A01A1_POWER1 0xC1   ///< Power Control 1
#define GC9A01A1_POWER2 0xC3   ///< Power Control 2
#define GC9A01A1_POWER3 0xC4   ///< Power Control 3
#define GC9A01A1_POWER4 0xC9   ///< Power Control 4
#define GC9A01A_RDID1 0xDA     ///< Read ID 1
#define GC9A01A_RDID2 0xDB     ///< Read ID 2
#define GC9A01A_RDID3 0xDC     ///< Read ID 3
#define GC9A01A_FRAMERATE 0xE8 ///< Frame rate control
#define GC9A01A_SPI2DATA 0xE9  ///< SPI 2DATA control
#define GC9A01A_INREGEN2 0xEF  ///< Inter register enable 2
#define GC9A01A_GAMMA1 0xF0    ///< Set gamma 1
#define GC9A01A_GAMMA2 0xF1    ///< Set gamma 2
#define GC9A01A_GAMMA3 0xF2    ///< Set gamma 3
#define GC9A01A_GAMMA4 0xF3    ///< Set gamma 4
#define GC9A01A_IFACE 0xF6     ///< Interface control
#define GC9A01A_INREGEN1 0xFE  ///< Inter register enable 1

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

void fd_gc9a01_exit_sleep(void) {
    uint8_t commands[] = {
        fd_spi_display_command(GC9A01A_SLPOUT),
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    k_sleep(K_MSEC(120)); // wait for exit sleep to complete
    fd_spi_display_cs_disable();
}

void fd_gc9a01_initialize(void) {
    fd_spi_display_initialize();
    fd_spi_display_reset();

    uint8_t id1;
    fd_spi_display_read(0xda, &id1, sizeof(id1));
    fd_assert(id1 == 0x00);
    uint8_t id2;
    fd_spi_display_read(0xdb, &id2, sizeof(id2));
    fd_assert(id2 == 0x9a);
    uint8_t id3;
    fd_spi_display_read(0xdc, &id3, sizeof(id3));
    fd_assert(id3 == 0x01);

    // !!! This doesn't seem to work, but the individual reads do... -denis
    uint8_t rdid[3];
    fd_spi_display_read(0x04, rdid, sizeof(rdid));

    uint8_t commands[] = {
        fd_spi_display_command(GC9A01A_INREGEN2),
        fd_spi_display_command(0xEB, 0x14),
        fd_spi_display_command(GC9A01A_INREGEN1),
        fd_spi_display_command(GC9A01A_INREGEN2),
        fd_spi_display_command(0xEB, 0x14),
        fd_spi_display_command(0x84, 0x40),
        fd_spi_display_command(0x85, 0xFF),
        fd_spi_display_command(0x86, 0xFF),
        fd_spi_display_command(0x87, 0xFF),
        fd_spi_display_command(0x88, 0x0A),
        fd_spi_display_command(0x89, 0x21),
        fd_spi_display_command(0x8A, 0x00),
        fd_spi_display_command(0x8B, 0x80),
        fd_spi_display_command(0x8C, 0x01),
        fd_spi_display_command(0x8D, 0x01),
        fd_spi_display_command(0x8E, 0xFF),
        fd_spi_display_command(0x8F, 0xFF),
        fd_spi_display_command(0xB6, 0x00, 0x20),
        fd_spi_display_command(GC9A01A_MADCTL, MADCTL_MX | MADCTL_BGR),
        fd_spi_display_command(GC9A01A_COLMOD, 0x06),
        fd_spi_display_command(0x90, 0x08, 0x08, 0x08, 0x08),
        fd_spi_display_command(0xBD, 0x06),
        fd_spi_display_command(0xBC, 0x00),
        fd_spi_display_command(0xFF, 0x60, 0x01, 0x04),
        fd_spi_display_command(GC9A01A1_POWER2, 0x13),
        fd_spi_display_command(GC9A01A1_POWER3, 0x13),
        fd_spi_display_command(GC9A01A1_POWER4, 0x22),
        fd_spi_display_command(0xBE, 0x11),
        fd_spi_display_command(0xE1, 0x10, 0x0E),
        fd_spi_display_command(0xDF, 0x21, 0x0c, 0x02),
        fd_spi_display_command(GC9A01A_GAMMA1, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A),
        fd_spi_display_command(GC9A01A_GAMMA2, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F),
        fd_spi_display_command(GC9A01A_GAMMA3, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A),
        fd_spi_display_command(GC9A01A_GAMMA4, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F),
        fd_spi_display_command(0xED, 0x1B, 0x0B),
        fd_spi_display_command(0xAE, 0x77),
        fd_spi_display_command(0xCD, 0x63),
        // Unsure what this line (from manufacturer's boilerplate code) is
        // meant to do, but users reported issues, seems to work OK without:
        // fd_spi_display_command(0x70, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03),
        fd_spi_display_command(GC9A01A_FRAMERATE, 0x34),
        fd_spi_display_command(0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70),
        fd_spi_display_command(0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70),
        fd_spi_display_command(0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07),
        fd_spi_display_command(0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00),
        fd_spi_display_command(0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98),
        fd_spi_display_command(0x74, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00),
        fd_spi_display_command(0x98, 0x3e, 0x07),
        fd_spi_display_command(GC9A01A_TEON),
        fd_spi_display_command(GC9A01A_INVON),
    };

    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();

    fd_gc9a01_exit_sleep();
    fd_gc9a01_display_on();
}

void fd_gc9a01_display_on(void) {
    uint8_t commands[] = {
        fd_spi_display_command(GC9A01A_DISPON),
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    k_sleep(K_MSEC(20));
    fd_spi_display_cs_disable();
}

void fd_gc9a01_display_off(void) {
    uint8_t commands[] = {
        fd_spi_display_command(GC9A01A_DISPOFF),
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}

void fd_gc9a01_write_image_start(int x, int y, int width, int height) {
    fd_assert(x >= 0);
    fd_assert((x + width) <= fd_gc9a01_width);
    fd_assert(y >= 0);
    fd_assert((y + height) <= fd_gc9a01_height);
    x += fd_gc9a01_x_shift;
    y += fd_gc9a01_y_shift;
    const int x1 = x + width - 1;
    const int y1 = y + height - 1;
    uint8_t commands[] = {
        fd_spi_display_command(GC9A01A_CASET, 0, x, 0, x1), // set column start and end address
        fd_spi_display_command(GC9A01A_RASET, 0, y, 0, y1), // set row start and end address
        fd_spi_display_command(GC9A01A_RAMWR), // write to RAM
    };
    fd_spi_display_cs_enable();
    fd_spi_display_write_commands(commands, sizeof(commands));
}

void fd_gc9a01_write_image_subdata(const uint8_t *data, size_t size) {
    fd_spi_display_write_data(data, size);
}

void fd_gc9a01_write_image_end(void) {
    uint8_t commands[] = {
    };
    fd_spi_display_write_commands(commands, sizeof(commands));
    fd_spi_display_cs_disable();
}
