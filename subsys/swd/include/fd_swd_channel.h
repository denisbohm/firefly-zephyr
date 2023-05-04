#ifndef fd_swd_channel_h
#define fd_swd_channel_h

#include <zephyr/drivers/gpio.h>

#include <stdbool.h>
#include <stdint.h>

typedef union {
    struct {
        uint32_t debug_port_bank_select:4;
        uint32_t access_port_bank_select:4;
        uint32_t reserved:16;
        uint32_t access_port_id:8;
    } fields;
    uint32_t value;
} fd_swd_channel_debug_port_access_t;

typedef struct {
    struct gpio_dt_spec gpio_power;
    struct gpio_dt_spec gpio_nreset;
    struct gpio_dt_spec gpio_direction;
    struct gpio_dt_spec gpio_clock;
    struct gpio_dt_spec gpio_data;
    struct gpio_dt_spec gpio_nreset_sense;

    bool overrun_detection_enabled;
    uint32_t ack_wait_retry_count;
    uint32_t register_retry_count;
    uint32_t half_bit_delay_ns;

    uint32_t target_id;
    fd_swd_channel_debug_port_access_t debug_port_access;
} fd_swd_channel_t;

#define fd_swd_channel_count 1

extern fd_swd_channel_t fd_swd_channels[];

void fd_swd_channel_initialize(void);

fd_swd_channel_t *fd_swd_channel_get(uint32_t index);

void fd_swd_channel_reinitialize(fd_swd_channel_t *channel);

void fd_swd_channel_set_power(fd_swd_channel_t *channel, bool power);

void fd_swd_channel_set_reset(fd_swd_channel_t *channel, bool nreset);

void fd_swd_channel_set_direction_to_read(fd_swd_channel_t *channel);

void fd_swd_channel_set_direction_to_write(fd_swd_channel_t *channel);

void fd_swd_channel_shift_out(fd_swd_channel_t *channel, uint8_t byte, int bit_count);

uint8_t fd_swd_channel_shift_in(fd_swd_channel_t *channel, int bit_count);

void fd_swd_channel_shift_out_bytes(fd_swd_channel_t *channel, uint8_t *data, uint32_t length);

void fd_swd_channel_shift_in_bytes(fd_swd_channel_t *channel, uint8_t *data, uint32_t length);

#endif
