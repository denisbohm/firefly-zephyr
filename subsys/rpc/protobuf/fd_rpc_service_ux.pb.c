
#include "fd_rpc_service_ux.pb.h"

#include "ux.pb.h"

const fd_rpc_method_t fd_rpc_service_ux_get_display_metadata = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 1,
    .request_size = firefly_ux_v1_GetDisplayMetadataRequest_size,
    .request_msgdesc = &firefly_ux_v1_GetDisplayMetadataRequest_msg,
    .response_size = firefly_ux_v1_GetDisplayMetadataResponse_size,
    .response_msgdesc = &firefly_ux_v1_GetDisplayMetadataResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_frame_buffer_read = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 2,
    .request_size = firefly_ux_v1_FrameBufferReadRequest_size,
    .request_msgdesc = &firefly_ux_v1_FrameBufferReadRequest_msg,
    .response_size = firefly_ux_v1_FrameBufferReadResponse_size,
    .response_msgdesc = &firefly_ux_v1_FrameBufferReadResponse_msg,
    .server_streaming = true,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_get_screen = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 3,
    .request_size = firefly_ux_v1_GetScreenRequest_size,
    .request_msgdesc = &firefly_ux_v1_GetScreenRequest_msg,
    .response_size = firefly_ux_v1_GetScreenResponse_size,
    .response_msgdesc = &firefly_ux_v1_GetScreenResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_set_screen = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 4,
    .request_size = firefly_ux_v1_SetScreenRequest_size,
    .request_msgdesc = &firefly_ux_v1_SetScreenRequest_msg,
    .response_size = firefly_ux_v1_SetScreenResponse_size,
    .response_msgdesc = &firefly_ux_v1_SetScreenResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_send_input_events = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 5,
    .request_size = firefly_ux_v1_SendInputEventsRequest_size,
    .request_msgdesc = &firefly_ux_v1_SendInputEventsRequest_msg,
    .response_size = firefly_ux_v1_SendInputEventsResponse_size,
    .response_msgdesc = &firefly_ux_v1_SendInputEventsResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_update = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 6,
    .request_size = firefly_ux_v1_UpdateRequest_size,
    .request_msgdesc = &firefly_ux_v1_UpdateRequest_msg,
    .response_size = firefly_ux_v1_UpdateResponse_size,
    .response_msgdesc = &firefly_ux_v1_UpdateResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_set_update_enabled = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 7,
    .request_size = firefly_ux_v1_SetUpdateEnabledRequest_size,
    .request_msgdesc = &firefly_ux_v1_SetUpdateEnabledRequest_msg,
    .response_size = firefly_ux_v1_SetUpdateEnabledResponse_size,
    .response_msgdesc = &firefly_ux_v1_SetUpdateEnabledResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_get_update_enabled = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 8,
    .request_size = firefly_ux_v1_GetUpdateEnabledRequest_size,
    .request_msgdesc = &firefly_ux_v1_GetUpdateEnabledRequest_msg,
    .response_size = firefly_ux_v1_GetUpdateEnabledResponse_size,
    .response_msgdesc = &firefly_ux_v1_GetUpdateEnabledResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_set_display_configuration = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 9,
    .request_size = firefly_ux_v1_SetDisplayConfigurationRequest_size,
    .request_msgdesc = &firefly_ux_v1_SetDisplayConfigurationRequest_msg,
    .response_size = firefly_ux_v1_SetDisplayConfigurationResponse_size,
    .response_msgdesc = &firefly_ux_v1_SetDisplayConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_get_display_configuration = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 10,
    .request_size = firefly_ux_v1_GetDisplayConfigurationRequest_size,
    .request_msgdesc = &firefly_ux_v1_GetDisplayConfigurationRequest_msg,
    .response_size = firefly_ux_v1_GetDisplayConfigurationResponse_size,
    .response_msgdesc = &firefly_ux_v1_GetDisplayConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_get_interaction_configuration = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 11,
    .request_size = firefly_ux_v1_GetInteractionConfigurationRequest_size,
    .request_msgdesc = &firefly_ux_v1_GetInteractionConfigurationRequest_msg,
    .response_size = firefly_ux_v1_GetInteractionConfigurationResponse_size,
    .response_msgdesc = &firefly_ux_v1_GetInteractionConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_ux_set_interaction_configuration = {
    .package_id = 9,
    .service_id = 1,
    .method_id = 12,
    .request_size = firefly_ux_v1_SetInteractionConfigurationRequest_size,
    .request_msgdesc = &firefly_ux_v1_SetInteractionConfigurationRequest_msg,
    .response_size = firefly_ux_v1_SetInteractionConfigurationResponse_size,
    .response_msgdesc = &firefly_ux_v1_SetInteractionConfigurationResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};
