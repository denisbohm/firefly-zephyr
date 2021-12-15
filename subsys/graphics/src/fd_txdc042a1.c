#include "fd_txdc042a1.h"

#include "fd_assert.h"
#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_txdc042a1_bus.h"
#include "fd_unused.h"

#include <string.h>

typedef struct {
    fd_txdc042a1_configuration_t configuration;
} fd_txdc042a1_t;

fd_txdc042a1_t fd_txdc042a1;

#define FD_TXDC042A1_SWRESET           0x12
#define FD_TXDC042A1_SET_X_WINDOW      0x44
#define FD_TXDC042A1_SET_Y_WINDOW      0x45
#define FD_TXDC042A1_WRITE_RAM         0x24
#define FD_TXDC042A1_MASTER_ACTIVATION 0x20

static bool fd_txdc042a1_is_busy(void) {
    return fd_gpio_get(fd_txdc042a1.configuration.busy);
}

static void fd_txdc042a1_cs_enable(void) {
    while (fd_txdc042a1_is_busy()) {
    }
    fd_gpio_set(fd_txdc042a1.configuration.csx, false);
}

static void fd_txdc042a1_cs_disable(void) {
    fd_gpio_set(fd_txdc042a1.configuration.csx, true);
}

static void fd_txdc042a1_command_mode(void) {
    fd_gpio_set(fd_txdc042a1.configuration.dcx, false);
}

static void fd_txdc042a1_data_mode(void) {
    fd_gpio_set(fd_txdc042a1.configuration.dcx, true);
}

static void fd_txdc042a1_write_command(uint8_t command) {
    fd_txdc042a1_command_mode();
    fd_txdc042a1_cs_enable();
    fd_txdc042a1_bus_write(&command, 1);
    fd_txdc042a1_cs_disable();
}

static void fd_txdc042a1_write_data(uint8_t data) {
    fd_txdc042a1_data_mode();
    fd_txdc042a1_cs_enable();
    fd_txdc042a1_bus_write(&data, 1);
    fd_txdc042a1_cs_disable();
}

static void fd_txdc042a1_reset(void) {
    fd_gpio_set(fd_txdc042a1.configuration.resx, true);
    fd_delay_ms(1);
    fd_gpio_set(fd_txdc042a1.configuration.resx, false);
    fd_delay_ms(1);
    fd_gpio_set(fd_txdc042a1.configuration.resx, true);
    fd_txdc042a1_write_command(FD_TXDC042A1_SWRESET);
}

static void fd_txdc042a1_set_x_window(int start, int count) {
    fd_assert(count > 0);
    fd_assert((start & 0b111) == 0);
    fd_assert((count & 0b111) == 0);

    int s = start / 8;
    int e = (start + count) / 8 - 1;
    fd_txdc042a1_write_command(0x44);
    fd_txdc042a1_write_data((uint8_t)s);
    fd_txdc042a1_write_data((uint8_t)e);

    fd_txdc042a1_write_command(0x4E);
    fd_txdc042a1_write_data((uint8_t)s);
}

static void fd_txdc042a1_set_y_window(int start, int count) {
    fd_assert(count > 0);

    int s = start;
    int e = start + count - 1;
    fd_txdc042a1_write_command(0x45);
    fd_txdc042a1_write_data((uint8_t)s);
    fd_txdc042a1_write_data((uint8_t)(s >> 8));
    fd_txdc042a1_write_data((uint8_t)e);
    fd_txdc042a1_write_data((uint8_t)(e >> 8));

    fd_txdc042a1_write_command(0x4F);
    fd_txdc042a1_write_data((uint8_t)s);
    fd_txdc042a1_write_data((uint8_t)(s >> 8));
}

static const uint8_t fd_txdc042a1_lut_data[] = {  //70 bytes + 6  bytes
    0x08,
    0x48,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00,
    0x08,
    0x48,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x48,
    0x48,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00,
    0x48,
    0x48,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0F,
    0x01,
    0x0F,
    0x01,
    0x00,
    0x0F,
    0x01,
    0x0F,
    0x01,
    0x00,
    0x0F,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

static void fd_txdc042a1_write_lut(void) {
    fd_txdc042a1_write_command(0x32);   // write LUT register
    for (int i = 0; i < 70; i++) {      // write LUT register 20160527
        fd_txdc042a1_write_data(fd_txdc042a1_lut_data[i]);
    }
}

static void fd_txdc042a1_fill(void) {
    fd_txdc042a1_write_command(0x4E);   // set RAM x address counter to 0
    fd_txdc042a1_write_data(0x00);

    fd_txdc042a1_write_command(0x4F);   // set RAM y address counter to 299
    fd_txdc042a1_write_data(0x00);
    fd_txdc042a1_write_data(0x00);
        
    fd_txdc042a1_write_command(0x24); // Write RAM

    for (int col = 0; col < 300; col++) {   // ◊‹π≤300 GATE
        for (int row = 0; row < 50; row++) {  // ◊‹π≤400 SOURCE£¨√ø∏ˆœÒÀÿ1bit,º¥ 400/8=50 ◊÷Ω⁄
            fd_txdc042a1_write_data(0xf0);
        }
    }

    fd_txdc042a1_write_command(0x20); // Activate Display Update Sequence
}

static void fd_txdc042a1_send_init_sequence(void) {
    fd_txdc042a1_reset();

  fd_txdc042a1_write_command(0x74);       // Set Analog Block Control
    fd_txdc042a1_write_data(0x54);    // 
  fd_txdc042a1_write_command(0x7E);       // Set Analog Block Control
    fd_txdc042a1_write_data(0x3B);    // Set Digital Block Control
  fd_txdc042a1_write_command(0x01);       // Driver Output control
    fd_txdc042a1_write_data(0x2B);    // A[8:0]=0x012B (Set Mux for 299£®0x012B£©+1=300)
    fd_txdc042a1_write_data(0x01);
    fd_txdc042a1_write_data(0x00);    // B[2]:GD=0[POR](G0 is the 1st gate output channel)  B[1]:SM=0[POR](left and right gate interlaced)  B[0]:TB=0[POR](scan from G0 to G319)

  fd_txdc042a1_write_command(0x0C);       // Booster Soft Start Setting
    fd_txdc042a1_write_data(0x8B);    // 0x8B
    fd_txdc042a1_write_data(0x9C);    // 0x9C
    fd_txdc042a1_write_data(0xD6);    // 0xD6 for str 6            
    fd_txdc042a1_write_data(0x0F);    // Set Digital Block Control

  fd_txdc042a1_write_command(0x3A);       // number of dummy line period   set dummy line for 50Hz frame freq
    fd_txdc042a1_write_data(0x2C);    // A[6:0]=0(Number of dummy line period in term of TGate)  
  fd_txdc042a1_write_command(0x3B);       // Gate line width   set gate line for 50Hz frame freq
    fd_txdc042a1_write_data(0x0A);    // A[3:0]=1001(68us)  Line width in us   68us*(???+0)=???us=???ms
  fd_txdc042a1_write_command(0x3C);       // board  ◊¢“‚£∫”Î÷Æ«∞µƒIC≤ª“ª—˘
    fd_txdc042a1_write_data(0x03);    // GS1-->GS1 LUT3  ø™ª˙µ⁄“ª¥ŒÀ¢–¬Border¥”∞◊µΩ∞◊

  fd_txdc042a1_write_command(0x11);       // data enter mode
    fd_txdc042a1_write_data(0x03);    // 01 ®CY decrement, X increment,
  fd_txdc042a1_write_command(0x44);       // set RAM x address start/end
    fd_txdc042a1_write_data(0x00);    // RAM x address start at 00h;
    fd_txdc042a1_write_data(0x31);    // RAM x address end at 31h (49+1)*8->400
  fd_txdc042a1_write_command(0x45);       // set RAM y address start/end
    fd_txdc042a1_write_data(0x00);    // RAM y address start at 012Bh+1=300;
    fd_txdc042a1_write_data(0x00);    // ∏ﬂŒªµÿ÷∑=01
    fd_txdc042a1_write_data(0x2B);    // RAM y address end at 00h;
    fd_txdc042a1_write_data(0x01);    // ∏ﬂŒªµÿ÷∑=0

  fd_txdc042a1_write_command(0x2C);     // vcom   ◊¢“‚£∫”Î÷Æ«∞µƒIC≤ª“ª—˘ ∑∂Œß£∫-0.2~-3V
    fd_txdc042a1_write_data(0x4C);    //-1.9V


  fd_txdc042a1_write_command(0x37);       // ¥Úø™«∞∫Û¡Ω∑˘Õº∂‘±»ƒ£ Ω
    fd_txdc042a1_write_data(0x00);    // 
    fd_txdc042a1_write_data(0x00);    // 
    fd_txdc042a1_write_data(0x00);    // 
    fd_txdc042a1_write_data(0x00);    // 
    fd_txdc042a1_write_data(0x80);    // 
  fd_txdc042a1_write_lut();
  fd_txdc042a1_write_command(0x21);       // ≤ªπ‹«∞“ª∑˘ª≠√Ê Option for Display Update  ◊¢“‚£∫”Î÷Æ«∞µƒIC≤ª“ª—˘      
    fd_txdc042a1_write_data(0x40);    // A[7:4] Red RAM option   A[3:0] BW RAM option  0000:Normal  0100:Bypass RAM content as 0( ˝æ›÷√0)    1000:Inverse RAM content ˝æ›0°¢1ª•ªª
  fd_txdc042a1_write_command(0x22);
    fd_txdc042a1_write_data(0xC7);    // (Enable Clock Signal, Enable CP) (Display update,Disable CP,Disable Clock Signal) ’˝≥£∏¸–¬À≥–Ú

  fd_txdc042a1_fill();         // INITIAL DISPLAY+»´∞◊µΩ»´∞◊ «Â∆¡£¨’‚—˘ø…∑¿÷πø™ª˙≥ˆœ÷ª®∆¡µƒŒ Ã‚

  fd_txdc042a1_write_command(0x21);       // Option for Display Update
    fd_txdc042a1_write_data(0x00);    // Normal  ∫Û√ÊÀ¢–¬ª÷∏¥’˝≥£µƒ∫⁄∞◊∫Ïœ‘ æ
  fd_txdc042a1_write_command(0x3C);       // board
    fd_txdc042a1_write_data(0xC0);    // VBD-->HiZ  ∫Û√ÊÀ¢–¬ ±Border∂º «∏ﬂ◊Ë
}

void fd_txdc042a1_initialize(fd_txdc042a1_configuration_t configuration) {
    memset(&fd_txdc042a1, 0, sizeof(fd_txdc042a1));
    fd_txdc042a1.configuration = configuration;

    fd_gpio_configure_output(fd_txdc042a1.configuration.resx, true);
    fd_gpio_set(fd_txdc042a1.configuration.resx, false);
    fd_gpio_set(fd_txdc042a1.configuration.resx, true);
    
    fd_gpio_configure_output(fd_txdc042a1.configuration.csx, true);
    fd_gpio_set(fd_txdc042a1.configuration.csx, false);
    fd_gpio_set(fd_txdc042a1.configuration.csx, true);

    fd_gpio_configure_output(fd_txdc042a1.configuration.dcx, true);
    fd_gpio_set(fd_txdc042a1.configuration.dcx, false);
    fd_gpio_set(fd_txdc042a1.configuration.dcx, true);

    fd_gpio_configure_input(fd_txdc042a1.configuration.busy);

    fd_txdc042a1_bus_initialize();

    fd_txdc042a1_send_init_sequence();
}

void fd_txdc042a1_write_image_start(int x, int y, int width, int height) {
    fd_assert(x >= 0);
    fd_assert(y >= 0);
    fd_assert(width > 0);
    fd_assert(height > 0);

    fd_txdc042a1_set_x_window(x, width);
    fd_txdc042a1_set_y_window(y, height);

    fd_txdc042a1_write_command(FD_TXDC042A1_WRITE_RAM);
}

void fd_txdc042a1_write_image_subdata(const uint8_t *data, int length) {
    for (int i = 0; i < length; ++i) {
        fd_txdc042a1_write_data(data[i]);
    }
}

void fd_txdc042a1_write_image_end(void) {
    fd_txdc042a1_write_command(FD_TXDC042A1_MASTER_ACTIVATION);
}
