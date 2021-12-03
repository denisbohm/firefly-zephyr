#ifndef fd_envelope_h
#define fd_envelope_h

#include "fd_binary.h"

#include <stdbool.h>
#include <stdint.h>

#define fd_envelope_system_firefly 0

#define fd_envelope_subsystem_system 0
#define fd_envelope_subsystem_i2cm   1
#define fd_envelope_subsystem_spim   2

#define fd_envelope_type_event    0
#define fd_envelope_type_request  1
#define fd_envelope_type_response 2

typedef struct {
    uint16_t crc16;
    uint16_t length;
    uint8_t target;
    uint8_t source;
    uint8_t system;
    uint8_t subsystem;
    uint8_t type;
    uint8_t reserved0;
} fd_envelope_t;

bool fd_envelope_decode(fd_binary_t *message, fd_envelope_t *envelope);
bool fd_envelope_encode(fd_binary_t *message, fd_envelope_t *envelope);

#endif