
#include "fd_rpc_service_rtc.pb.h"

#include "rtc.pb.h"

const fd_rpc_method_t fd_rpc_service_rtc_get_time = {
    .package_id = 10,
    .service_id = 1,
    .method_id = 1,
    .request_size = firefly_rtc_v1_GetTimeRequest_size,
    .request_msgdesc = &firefly_rtc_v1_GetTimeRequest_msg,
    .response_size = firefly_rtc_v1_GetTimeResponse_size,
    .response_msgdesc = &firefly_rtc_v1_GetTimeResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_rtc_set_time = {
    .package_id = 10,
    .service_id = 1,
    .method_id = 2,
    .request_size = firefly_rtc_v1_SetTimeRequest_size,
    .request_msgdesc = &firefly_rtc_v1_SetTimeRequest_msg,
    .response_size = firefly_rtc_v1_SetTimeResponse_size,
    .response_msgdesc = &firefly_rtc_v1_SetTimeResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_rtc_get_configuration = {
    .package_id = 10,
    .service_id = 1,
    .method_id = 3,
    .request_size = firefly_rtc_v1_GetConfigurationRequest_size,
    .request_msgdesc = &firefly_rtc_v1_GetConfigurationRequest_msg,
    .response_size = firefly_rtc_v1_GetConfigurationResponse_size,
    .response_msgdesc = &firefly_rtc_v1_GetConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_rtc_set_configuration = {
    .package_id = 10,
    .service_id = 1,
    .method_id = 4,
    .request_size = firefly_rtc_v1_SetConfigurationRequest_size,
    .request_msgdesc = &firefly_rtc_v1_SetConfigurationRequest_msg,
    .response_size = firefly_rtc_v1_SetConfigurationResponse_size,
    .response_msgdesc = &firefly_rtc_v1_SetConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};
