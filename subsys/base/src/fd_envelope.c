#include "fd_envelope.h"

#include "fd_cobs.h"
#include "fd_crc16.h"

#define fd_envelope_size 10

bool fd_envelope_decode(fd_binary_t *message, fd_envelope_t *envelope) {
    message->put_index = (uint32_t)fd_cobs_decode(message->buffer, message->put_index);
    if (message->put_index < fd_envelope_size) {
        return false;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, &message->buffer[message->put_index - fd_envelope_size], fd_envelope_size);
    envelope->reserved0 = fd_binary_get_uint8(&binary);
    envelope->type = fd_binary_get_uint8(&binary);
    envelope->subsystem = fd_binary_get_uint8(&binary);
    envelope->system = fd_binary_get_uint8(&binary);
    envelope->source = fd_binary_get_uint8(&binary);
    envelope->target = fd_binary_get_uint8(&binary);
    envelope->length = fd_binary_get_uint16(&binary);
    envelope->crc16 = fd_binary_get_uint16(&binary);

    uint16_t crc16 = fd_crc16_update(message->buffer, message->put_index - 2);
    if (crc16 != envelope->crc16) {
        return false;
    }

    uint16_t length = message->put_index - fd_envelope_size;
    if (length != envelope->length) {
        return false;
    }

    message->put_index -= fd_envelope_size;
    return true;
}

bool fd_envelope_encode(fd_binary_t *message, fd_envelope_t *envelope) {
    envelope->length = (uint16_t)message->put_index;
    fd_binary_put_uint8(message, envelope->reserved0);
    fd_binary_put_uint8(message, envelope->type);
    fd_binary_put_uint8(message, envelope->subsystem);
    fd_binary_put_uint8(message, envelope->system);
    fd_binary_put_uint8(message, envelope->source);
    fd_binary_put_uint8(message, envelope->target);
    fd_binary_put_uint16(message, envelope->length);
    envelope->crc16 = fd_crc16_update(message->buffer, message->put_index);
    fd_binary_put_uint16(message, envelope->crc16);
    uint8_t buffer[32];
    size_t length = fd_cobs_encode(message->buffer, message->put_index, message->size, buffer, sizeof(buffer));
    if (length == 0) {
        message->errors |= fd_binary_error_overflow;
    }
    message->put_index = (uint32_t)length;
    return message->errors == 0;
}
