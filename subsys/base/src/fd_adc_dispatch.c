#include "fd_adc_dispatch.h"

#include "fd_adc.h"
#include "fd_adc_operation.h"
#include "fd_assert.h"
#include "fd_assert_log.h"
#include "fd_dispatch.h"

#include <string.h>

bool fd_adc_dispatch_convert(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint32_t channel = fd_binary_get_uint32(message);
    float max_voltage = fd_binary_get_float32(message);
    float voltage = fd_adc_convert(channel, max_voltage);
    uint8_t buffer[64];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, fd_adc_operation_convert);
    fd_binary_put_uint32(&response, channel);
    fd_binary_put_float32(&response, voltage);
    fd_envelope_t response_envelope = {
        .target = envelope->source,
        .source = envelope->target,
        .system = envelope->system,
        .subsystem = envelope->subsystem,
        .type = fd_envelope_type_response,
    };
    return respond(&response, &response_envelope);
}

bool fd_adc_dispatch_process(fd_binary_t *message, fd_envelope_t *envelope, fd_dispatch_respond_t respond) {
    uint8_t operation = fd_binary_get_uint8(message);
    switch (operation) {
        case fd_adc_operation_convert:
            return fd_adc_dispatch_convert(message, envelope, respond);
        default:
        return false;
    }
}

bool fd_adc_dispatch_filter(const fd_envelope_t *envelope) {
    return
        (envelope->system == fd_envelope_system_firefly) &&
        (envelope->subsystem == fd_envelope_subsystem_adc);
}

void fd_adc_dispatch_initialize(void) {
    fd_assert_log_initialize();

    fd_dispatch_add_process(fd_adc_dispatch_process, fd_adc_dispatch_filter);
}