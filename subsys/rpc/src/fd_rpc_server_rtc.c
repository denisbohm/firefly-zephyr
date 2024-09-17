#include "fd_rpc_server_rtc.h"

#include "fd_assert.h"
#include "fd_rpc.h"
#include "fd_rpc_service_rtc.pb.h"
#include "fd_rtc.h"
#include "fd_unused.h"

#include "rtc.pb.h"

fd_source_push()

bool fd_rpc_server_rtc_get_time_request(fd_rpc_server_context_t *context, const void *a_request fd_unused) {
    firefly_rtc_v1_GetTimeResponse response = {
        .is_set = fd_rtc_is_set(),
        .utc = fd_rtc_get_utc(),
        .us = 0,
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_rtc_set_time_request(fd_rpc_server_context_t *context, const void *a_request fd_unused) {
    const firefly_rtc_v1_SetTimeRequest *request = a_request;
    fd_rtc_set_utc(request->utc);
    firefly_rtc_v1_SetTimeResponse response = {};
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_rtc_get_configuration_request(fd_rpc_server_context_t *context, const void *a_request fd_unused) {
    fd_rtc_configuration_t configuration;
    fd_rtc_get_configuration(&configuration);
    firefly_rtc_v1_GetConfigurationResponse response = {
        .has_configuration = true,
        .configuration = {
            .time_zone_offset = configuration.time_zone_offset,
            .display_format = (firefly_rtc_v1_DisplayFormat)configuration.display_format,
        }
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_rtc_set_configuration_request(fd_rpc_server_context_t *context, const void *a_request fd_unused) {
    const firefly_rtc_v1_SetConfigurationRequest *request = a_request;
    fd_rtc_configuration_t configuration = {
        .display_format = (fd_rtc_display_format_t)request->configuration.display_format,
        .time_zone_offset = request->configuration.time_zone_offset,
    };
    fd_rtc_set_configuration(&configuration);
    firefly_rtc_v1_SetTimeResponse response = {};
    return fd_rpc_server_send_client_response(context, &response);
}

void fd_rpc_server_rtc_initialize(void) {
    static const fd_rpc_method_server_t get_time_server = {
        .request = fd_rpc_server_rtc_get_time_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_rtc_get_time, &get_time_server);

    static const fd_rpc_method_server_t set_time_server = {
        .request = fd_rpc_server_rtc_set_time_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_rtc_set_time, &set_time_server);

    static const fd_rpc_method_server_t get_configuration_server = {
        .request = fd_rpc_server_rtc_get_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_rtc_get_configuration, &get_configuration_server);

    static const fd_rpc_method_server_t set_configuration_server = {
        .request = fd_rpc_server_rtc_set_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_rtc_set_configuration, &set_configuration_server);
}

fd_source_pop()
