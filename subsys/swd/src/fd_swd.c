#include "fd_swd.h"

#include "fd_assert.h"

#include <stddef.h>
#include <string.h>

bool fd_swd_error_return(fd_swd_error_t *error, uint64_t code, uint64_t detail) {
    if (error) {
        error->code = code;
        error->detail = detail;
    }
    return false;
}

const uint64_t fd_swd_error_unexpected_ack = 1;
const uint64_t fd_swd_error_too_many_wait_retries = 2;
const uint64_t fd_swd_error_sticky = 3;
const uint64_t fd_swd_error_parity = 4;
const uint64_t fd_swd_error_mismatch = 5;
const uint64_t fd_swd_error_invalid = 6;
const uint64_t fd_swd_error_not_ready = 7;

void fd_swd_reset_debug_port(fd_swd_channel_t *channel) {
    fd_swd_channel_set_direction_to_write(channel);
    uint8_t bytes[] = {
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0x9e,
        0xe7,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0x00,
    };
    fd_swd_channel_shift_out_bytes(channel, bytes, sizeof(bytes));
}

void fd_swd_flush(fd_swd_channel_t *channel) {
    uint8_t data = 0;
    fd_swd_channel_shift_out_bytes(channel, &data, 1);
}

uint8_t fd_swd_get_parity_uint8(uint8_t v) {
    return (0x6996 >> ((v ^ (v >> 4)) & 0xf)) & 1;
}

uint8_t fd_swd_get_parity_uint32(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    return fd_swd_get_parity_uint8(v);
}

uint8_t fd_swd_encode_request(fd_swd_port_t port, fd_swd_direction_t direction, uint8_t address) {
    uint8_t request = 0b10000001; // Start (bit 0) & Park (bit 7)
    if (port == fd_swd_port_access) {
        request |= 0b00000010;
    }
    if (direction == fd_swd_direction_read) {
        request |= 0b00000100;
    }
    request |= (address << 1) & 0b00011000;
    if (fd_swd_get_parity_uint8(request)) {
        request |= 0b00100000;
    }
    return request;
}

fd_swd_ack_t fd_swd_request(fd_swd_channel_t *channel, uint8_t request) {
    fd_swd_channel_shift_out_bytes(channel, &request, 1);
    fd_swd_channel_set_direction_to_read(channel);
    uint8_t ack = fd_swd_channel_shift_in(channel, 4) >> 5;
    if (ack != 1) {
        static int errors = 0;
        ++errors;
    }
    return ack;
}

bool fd_swd_read_uint32(fd_swd_channel_t *channel, uint32_t *value) {
    if (channel->half_bit_delay_ns == 0) {
        fd_swd_channel_shift_in_bytes(channel, (uint8_t *)value, sizeof(uint32_t));
    } else {
        uint32_t byte_0 = fd_swd_channel_shift_in(channel, 8);
        uint32_t byte_1 = fd_swd_channel_shift_in(channel, 8);
        uint32_t byte_2 = fd_swd_channel_shift_in(channel, 8);
        uint32_t byte_3 = fd_swd_channel_shift_in(channel, 8);
        *value = (byte_3 << 24) | (byte_2 << 16) | (byte_1 << 8) | byte_0;
    }
    uint8_t parity = fd_swd_channel_shift_in(channel, 1) >> 7;
    if (parity != fd_swd_get_parity_uint32(*value)) {
        return false;
    }
    return true;
}

void fd_swd_write_uint32(fd_swd_channel_t *channel, uint32_t value) {
    if (channel->half_bit_delay_ns == 0) {
        fd_swd_channel_shift_out_bytes(channel, (uint8_t *)&value, sizeof(uint32_t));
    } else {
        fd_swd_channel_shift_out(channel, value, 8);
        fd_swd_channel_shift_out(channel, value >> 8, 8);
        fd_swd_channel_shift_out(channel, value >> 16, 8);
        fd_swd_channel_shift_out(channel, value >> 24, 8);
    }
    uint8_t parity = fd_swd_get_parity_uint32(value);
    fd_swd_channel_shift_out(channel, parity, 1);
}

void fd_swd_turn_to_write_and_skip(fd_swd_channel_t *channel) {
    fd_swd_channel_shift_in(channel, 1);
    fd_swd_channel_set_direction_to_write(channel);
}

bool fd_swd_read_port(
    fd_swd_channel_t *channel,
    fd_swd_port_t port,
    uint8_t register_offset,
    uint32_t *value,
    fd_swd_error_t *error
) {
    bool overrun_detection_enabled = channel->overrun_detection_enabled;
    uint32_t retry_count = channel->ack_wait_retry_count;

    fd_swd_ack_t ack = fd_swd_ack_ok;
    uint8_t request = fd_swd_encode_request(port, fd_swd_direction_read, register_offset);
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        ack = fd_swd_request(channel, request);
        if (overrun_detection_enabled) {
            if (!fd_swd_read_uint32(channel, value)) {
                continue;
            }
            fd_swd_turn_to_write_and_skip(channel);
        }
        if (ack == fd_swd_ack_ok) {
            if (!overrun_detection_enabled) {
                if (!fd_swd_read_uint32(channel, value)) {
                    continue;
                }
                fd_swd_turn_to_write_and_skip(channel);
            }
            return true;
        }
        if (!overrun_detection_enabled) {
            fd_swd_turn_to_write_and_skip(channel);
        }
        if (ack != fd_swd_ack_wait) {
            return fd_swd_error_return(error, fd_swd_error_unexpected_ack, ack);
        }
    }
    fd_assert(ack == fd_swd_ack_wait);
    return fd_swd_error_return(error, fd_swd_error_too_many_wait_retries, ack);
}

bool fd_swd_write_port(
    fd_swd_channel_t *channel,
    fd_swd_port_t port,
    uint8_t register_offset,
    uint32_t value,
    fd_swd_error_t *error
) {
    bool overrun_detection_enabled = channel->overrun_detection_enabled;
    uint32_t retry_count = channel->ack_wait_retry_count;

    fd_swd_ack_t ack = fd_swd_ack_ok;
    uint8_t request = fd_swd_encode_request(port, fd_swd_direction_write, register_offset);
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        ack = fd_swd_request(channel, request);
        fd_swd_turn_to_write_and_skip(channel);
        if (overrun_detection_enabled) {
            fd_swd_write_uint32(channel, value);
        }
        if (ack == fd_swd_ack_ok) {
            if (!overrun_detection_enabled) {
                fd_swd_write_uint32(channel, value);
            }
            return true;
        }
        if (ack != fd_swd_ack_wait) {
            return fd_swd_error_return(error, fd_swd_error_unexpected_ack, ack);
        }
    }
    fd_assert(ack == fd_swd_ack_wait);
    return fd_swd_error_return(error, fd_swd_error_too_many_wait_retries, ack);
}

bool fd_swd_read_debug_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t *value,
    fd_swd_error_t *error
) {
    return fd_swd_read_port(channel, fd_swd_port_debug, register_offset, value, error);
}

bool fd_swd_write_debug_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t value,
    fd_swd_error_t *error
) {
    return fd_swd_write_port(channel, fd_swd_port_debug, register_offset, value, error);
}

bool fd_swd_read_access_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t *value,
    fd_swd_error_t *error
) {
    return fd_swd_read_port(channel, fd_swd_port_access, register_offset, value, error);
}

bool fd_swd_write_access_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t value,
    fd_swd_error_t *error
) {
    return fd_swd_write_port(channel, fd_swd_port_access, register_offset, value, error);
}

void fd_swd_set_target_id(fd_swd_channel_t *channel, uint32_t target_id) {
    channel->target_id = target_id;
}

void fd_swd_select_access_port_id(fd_swd_channel_t *channel, uint8_t access_port_id) {
    channel->debug_port_access.fields.access_port_id = access_port_id;
}

void fd_swd_select_access_port_bank_select(fd_swd_channel_t *channel, uint8_t access_port_bank_select) {
    channel->debug_port_access.fields.access_port_bank_select = access_port_bank_select;
}

void fd_swd_select_access_port_register(fd_swd_channel_t *channel, uint8_t access_port_register) {
    fd_swd_select_access_port_bank_select(channel, (access_port_register >> 4) & 0xf);
}

void fd_swd_select_debug_port_bank_select(fd_swd_channel_t *channel, uint8_t debug_port_bank_select) {
    channel->debug_port_access.fields.debug_port_bank_select = debug_port_bank_select;
}

void fd_swd_select_debug_port_register(fd_swd_channel_t *channel, uint8_t debug_port_register) {
    fd_swd_select_debug_port_bank_select(channel, (debug_port_register >> 4) & 0xf);
}

bool fd_swd_port_select(fd_swd_channel_t *channel, fd_swd_error_t *error) {
    uint32_t value = channel->debug_port_access.value;
    return fd_swd_write_port(channel, fd_swd_port_debug, FD_SWD_DP_SELECT, value, error);
}

bool fd_swd_select_and_read_access_port(
    fd_swd_channel_t *channel,
    uint8_t access_port_register,
    uint32_t *value,
    fd_swd_error_t *error
) {
    fd_swd_select_access_port_register(channel, access_port_register);
    if (!fd_swd_port_select(channel, error)) {
        return false;
    }
    uint32_t dummy;
    if (!fd_swd_read_port(channel, fd_swd_port_access, access_port_register, &dummy, error)) {
        return false;
    }
    if (!fd_swd_read_port(channel, fd_swd_port_debug, FD_SWD_DP_RDBUFF, value, error)) {
        return false;
    }
    return true;
}

bool fd_swd_select_and_write_access_port(
    fd_swd_channel_t *channel,
    uint8_t access_port_register,
    uint32_t value,
    fd_swd_error_t *error
) {
    fd_swd_select_access_port_register(channel, access_port_register);
    if (!fd_swd_port_select(channel, error)) {
        return false;
    }
    if (!fd_swd_write_port(channel, fd_swd_port_access, access_port_register, value, error)) {
        return false;
    }
    fd_swd_flush(channel);
    return true;
}

bool fd_swd_set_overrun_detection(
    fd_swd_channel_t *channel,
    bool enabled,
    fd_swd_error_t *error
) {
    const uint32_t flags =
        FD_SWD_DP_ABORT_ORUNERRCLR |
        FD_SWD_DP_ABORT_WDERRCLR |
        FD_SWD_DP_ABORT_STKERRCLR |
        FD_SWD_DP_ABORT_STKCMPCLR;
    if (!fd_swd_write_debug_port(channel, FD_SWD_DP_ABORT, flags, error)) {
        return false;
    }
    
    uint32_t value = FD_SWD_DP_CTRL_CDBGPWRUPREQ | FD_SWD_DP_CTRL_CSYSPWRUPREQ;
    if (enabled) {
        value |= FD_SWD_DP_STAT_ORUNDETECT;
    }
    if (!fd_swd_write_debug_port(channel, FD_SWD_DP_CTRL, value, error)) {
        return false;
    }
    
    channel->overrun_detection_enabled = enabled;
    return true;
}

#define tar_increment_bits 0x3f

bool fd_swd_before_memory_transfer(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint32_t length,
    fd_swd_error_t *error
) {
    if ((address & 0x3) != 0) {
        return fd_swd_error_return(error, fd_swd_error_invalid, address);
    }
    if ((length == 0) || ((length & 0x3) != 0)) {
        return fd_swd_error_return(error, fd_swd_error_invalid, length);
    }
    // TAR auto increment is only guaranteed in the first 10-bits (beyond that is implementation defined)
    uint32_t end_address = address + length - 1;
    if ((address & ~tar_increment_bits) != (end_address & ~tar_increment_bits)) {
        return fd_swd_error_return(error, fd_swd_error_invalid, length);
    }

    if (!fd_swd_select_and_write_access_port(channel, FD_SWD_AP_TAR, address, error)) {
        return false;
    }
    fd_swd_select_access_port_register(channel, FD_SWD_AP_DRW);
    if (!fd_swd_port_select(channel, error)) {
        return false;
    }
    return fd_swd_set_overrun_detection(channel, true, error);
}

bool fd_swd_after_memory_transfer(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
) {
    uint32_t status;
    if (!fd_swd_read_debug_port(channel, FD_SWD_DP_STAT, &status, error)) {
        return false;
    }
    if (!fd_swd_set_overrun_detection(channel, false, error)) {
        return false;
    }
    if (status & (FD_SWD_DP_STAT_WDATAERR | FD_SWD_DP_STAT_STICKYERR | FD_SWD_DP_STAT_STICKYORUN)) {
        return fd_swd_error_return(error, fd_swd_error_sticky, status);
    }
    return true;
}

bool fd_swd_request_write(fd_swd_channel_t *channel, uint8_t request, uint32_t value, fd_swd_error_t *error) {
    fd_swd_channel_shift_out_bytes(channel, &request, 1);
    fd_swd_channel_set_direction_to_read(channel);
    fd_swd_ack_t ack = fd_swd_channel_shift_in(channel, 4) >> 5;
    // !!! is this the right error handling?  to bail at this point or finish the sequence below? -denis
    if (ack != fd_swd_ack_ok) {
        return fd_swd_error_return(error, fd_swd_error_unexpected_ack, ack);
    }
    fd_swd_turn_to_write_and_skip(channel);
    fd_swd_write_uint32(channel, value);
    return true;
}

bool fd_swd_request_read(fd_swd_channel_t *channel, uint8_t request, uint32_t *value, fd_swd_error_t *error) {
    fd_swd_channel_shift_out_bytes(channel, &request, 1);
    fd_swd_channel_set_direction_to_read(channel);
    fd_swd_ack_t ack = fd_swd_channel_shift_in(channel, 4) >> 5;
    // !!! is this the right error handling?  to bail at this point or finish the sequence below? -denis
    if (ack != fd_swd_ack_ok) {
        return fd_swd_error_return(error, fd_swd_error_unexpected_ack, ack);
    }
    if (!fd_swd_read_uint32(channel, value)) {
        return fd_swd_error_return(error, fd_swd_error_parity, *value);
    }
    fd_swd_turn_to_write_and_skip(channel);
    return true;
}

static uint32_t unpack_little_endian_uint32(uint8_t *bytes) {
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

static void pack_little_endian_uint32(uint8_t *bytes, uint32_t value) {
    bytes[0] = value;
    bytes[1] = value >> 8;
    bytes[2] = value >> 16;
    bytes[3] = value >> 24;
}

bool fd_swd_write_memory_transfer(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
) {
    if (!fd_swd_before_memory_transfer(channel, address, length, error)) {
        return false;
    }
    
    bool success = true;
    uint8_t request = fd_swd_encode_request(fd_swd_port_access, fd_swd_direction_write, FD_SWD_AP_DRW);
    for (uint32_t i = 0; i < length; i += 4) {
        if (!fd_swd_request_write(channel, request, unpack_little_endian_uint32(&data[i]), error)) {
            success = false;
            break;
        }
    }
    
    if (!fd_swd_after_memory_transfer(channel, error)) {
        return false;
    }

    return success;
}

bool fd_swd_read_memory_transfer(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
) {
    if (!fd_swd_before_memory_transfer(channel, address, length, error)) {
        return false;
    }
    
    uint8_t request = fd_swd_encode_request(fd_swd_port_access, fd_swd_direction_read, FD_SWD_AP_DRW);
    uint32_t words = length / 4;
    uint32_t index = 0;
    bool success = true;
    // note: 1 extra iteration because of 1 read delay in getting data out
    uint32_t value;
    if (!fd_swd_request_read(channel, request, &value, error)) {
        success = false;
    } else {
        for (uint32_t i = 0; i < words; ++i) {
            if (!fd_swd_request_read(channel, request, &value, error)) {
                success = false;
                break;
            }
            pack_little_endian_uint32(&data[index], value);
            index += 4;
        }
    }
    
    if (!fd_swd_after_memory_transfer(channel, error)) {
        return false;
    }

    return success;
}

bool fd_swd_write_data(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
) {
    uint32_t offset = 0;
    while (length > 0) {
        uint32_t sublength = (tar_increment_bits + 1) - (address & tar_increment_bits);
        if (length < sublength) {
            sublength = length;
        }
        
        if (!fd_swd_write_memory_transfer(channel, address, data, sublength, error)) {
            return false;
        }
        
        address += sublength;
        data += sublength;
        length -= sublength;
        offset += sublength;
    }
    return true;
}

bool fd_swd_read_data(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
) {
    uint32_t offset = 0;
    while (length > 0) {
        uint32_t sublength = (tar_increment_bits + 1) - (address & tar_increment_bits);
        if (length < sublength) {
            sublength = length;
        }
        
        if (!fd_swd_read_memory_transfer(channel, address, data, sublength, error)) {
            return false;
        }
        
        address += sublength;
        data += sublength;
        length -= sublength;
        offset += sublength;
    }
    return true;
}

#define FD_SWD_DP_SELECT_APSEL_NRF52_CTRL_AP 1

#define NRF52_AP_REG_RESET 0x00
#define NRF52_AP_REG_ERASEALL 0x04
#define NRF52_AP_REG_ERASEALLSTATUS 0x08
#define NRF52_AP_REG_APPROTECTSTATUS 0x0c
#define NRF52_AP_REG_IDR 0xfc

#define NRF52_CTRL_AP_ID 0x02880000

bool fd_swd_wait_for_debug_port_status(
    fd_swd_channel_t *channel,
    uint32_t mask,
    fd_swd_error_t *error
) {
    int debug_port_status_retry_count = 3;
    uint32_t status = 0;
    for (int retry = 0; retry < debug_port_status_retry_count; ++retry) {
        if (!fd_swd_read_debug_port(channel, FD_SWD_DP_STAT, &status, error)) {
            return false;
        }
        if (status & mask) {
            return true;
        }
    }
    return fd_swd_error_return(error, fd_swd_error_too_many_wait_retries, status);
}

bool fd_swd_initialize_debug_port(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
) {
    uint32_t stat;
    if (!fd_swd_read_debug_port(channel, FD_SWD_DP_STAT, &stat, error)) {
        return false;
    }
    
    if (!fd_swd_write_debug_port(channel, FD_SWD_DP_ABORT,
                              FD_SWD_DP_ABORT_ORUNERRCLR |
                              FD_SWD_DP_ABORT_WDERRCLR |
                              FD_SWD_DP_ABORT_STKERRCLR |
                              FD_SWD_DP_ABORT_STKCMPCLR, error)
    ) {
        return false;
    }

    if (!fd_swd_read_debug_port(channel, FD_SWD_DP_STAT, &stat, error)) {
        return false;
    }

    if (!fd_swd_write_debug_port(channel, FD_SWD_DP_CTRL, FD_SWD_DP_CTRL_CDBGPWRUPREQ | FD_SWD_DP_CTRL_CSYSPWRUPREQ, error)) {
        return false;
    }

    if (!fd_swd_wait_for_debug_port_status(channel, FD_SWD_DP_CTRL_CSYSPWRUPACK, error)) {
        return false;
    }

    if (!fd_swd_wait_for_debug_port_status(channel, FD_SWD_DP_CTRL_CDBGPWRUPACK, error)) {
        return false;
    }

    if (!fd_swd_write_debug_port(channel, FD_SWD_DP_SELECT, 0, error)) {
        return false;
    }

    if (!fd_swd_read_debug_port(channel, FD_SWD_DP_STAT, &stat, error)) {
        return false;
    }

    uint32_t dpid;
    if (!fd_swd_read_debug_port(channel, FD_SWD_DP_DPIDR, &dpid, error)) {
        return false;
    }

    return true;
}

bool fd_swd_initialize_access_port(fd_swd_channel_t *channel, fd_swd_error_t *error) {
  return fd_swd_select_and_write_access_port(
      channel,
      FD_SWD_AP_CSW,
      FD_SWD_AP_CSW_PROT | FD_SWD_AP_CSW_ADDRINC_SINGLE | FD_SWD_AP_CSW_SIZE_32BIT,
      error);
}

bool fd_swd_read_memory_uint32(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint32_t *value,
    fd_swd_error_t *error
) {
    if (!fd_swd_select_and_write_access_port(channel, FD_SWD_AP_TAR, address, error)) {
        return false;
    }
    uint32_t tar;
    if (!fd_swd_select_and_read_access_port(channel, FD_SWD_AP_TAR, &tar, error)) {
        return false;
    }
    return fd_swd_select_and_read_access_port(channel, FD_SWD_AP_DRW, value, error);
}

bool fd_swd_write_memory_uint32(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint32_t value,
    fd_swd_error_t *error
) {
    if (!fd_swd_select_and_write_access_port(channel, FD_SWD_AP_TAR, address, error)) {
        return false;
    }
    return fd_swd_select_and_write_access_port(channel, FD_SWD_AP_DRW, value, error);
}

bool fd_swd_wait_for_register_ready(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
) {
    uint32_t dhcsr = 0xf;
    uint32_t retry_count = channel->register_retry_count;
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        if (!fd_swd_read_memory_uint32(channel, FD_SWD_MEMORY_DHCSR, &dhcsr, error)) {
            return false;
        }
        if (dhcsr & FD_SWD_DHCSR_STAT_REGRDY) {
            return true;
        }
    }
    return fd_swd_error_return(error, fd_swd_error_not_ready, 0);
}

bool fd_swd_read_register(
    fd_swd_channel_t *channel,
    uint16_t register_id,
    uint32_t *value,
    fd_swd_error_t *error
) {
    if (!fd_swd_write_memory_uint32(channel, FD_SWD_MEMORY_DCRSR, register_id, error)) {
        return false;
    }
    if (!fd_swd_wait_for_register_ready(channel, error)) {
        return false;
    }
    if (!fd_swd_read_memory_uint32(channel, FD_SWD_MEMORY_DCRDR, value, error)) {
        return false;
    }
    return true;
}

bool fd_swd_write_register(
    fd_swd_channel_t *channel,
    uint16_t register_id,
    uint32_t value,
    fd_swd_error_t *error
) {
    if (!fd_swd_write_memory_uint32(channel, FD_SWD_MEMORY_DCRDR, value, error)) {
        return false;
    }
    if (!fd_swd_write_memory_uint32(channel, FD_SWD_MEMORY_DCRSR, 0x00010000 | register_id, error)) {
        return false;
    }
    if (!fd_swd_wait_for_register_ready(channel, error)) {
        return false;
    }
    return true;
}

bool fd_swd_is_halted(fd_swd_channel_t *channel, bool *halted, fd_swd_error_t *error) {
    uint32_t dhcsr;
    if (!fd_swd_read_memory_uint32(channel, FD_SWD_MEMORY_DHCSR, &dhcsr, error)) {
        return false;
    }
    *halted = (dhcsr & FD_SWD_DHCSR_STAT_HALT) != 0;
    return true;
}

bool fd_swd_write_DHCSR(fd_swd_channel_t *channel, uint32_t value, fd_swd_error_t *error) {
    value |= FD_SWD_DHCSR_DBGKEY | FD_SWD_DHCSR_CTRL_DEBUGEN;
    return fd_swd_write_memory_uint32(channel, FD_SWD_MEMORY_DHCSR, value, error);
}

bool fd_swd_halt(fd_swd_channel_t *channel, fd_swd_error_t *error) {
    return fd_swd_write_DHCSR(channel, FD_SWD_DHCSR_CTRL_HALT, error);
}

bool fd_swd_step(fd_swd_channel_t *channel, fd_swd_error_t *error) {
    return fd_swd_write_DHCSR(channel, FD_SWD_DHCSR_CTRL_STEP, error);
}

bool fd_swd_run(fd_swd_channel_t *channel, fd_swd_error_t *error) {
    return fd_swd_write_DHCSR(channel, 0, error);
}

bool fd_swd_connect(
    fd_swd_channel_t *channel,
    uint32_t *dpid,
    fd_swd_error_t *error
) {
    fd_swd_reset_debug_port(channel);

    if (channel->target_id != 0) {
        fd_swd_error_t error;
        memset(&error, 0, sizeof(error));
        uint32_t ack_wait_retry_count = channel->ack_wait_retry_count;
        channel->ack_wait_retry_count = 1;
        fd_swd_write_debug_port(channel, FD_SWD_DP_TARGETSEL, channel->target_id, &error);
        channel->ack_wait_retry_count = ack_wait_retry_count;
    }

    bool success = fd_swd_read_debug_port(channel, FD_SWD_DP_DPIDR, dpid, error);

    if (success) {
        success = fd_swd_initialize_debug_port(channel, error);
    }

    return success;
}