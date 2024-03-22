#include "fd_rpc_server_ux.h"

#include "fd_ux.h"
#include "fd_rpc.h"

#include "fd_rpc_service_ux.pb.h"
#include "ux.pb.h"

#include <fd_assert.h>
#include <fd_ux_button.h>
#include <fd_unused.h>
#include <fd_ux.h>

fd_source_push()

#define fd_rpc_server_ux_frame_buffer_read_min_free_space (firefly_ux_v1_FrameBufferReadResponse_size + 32 /* rpc overhead */)

typedef struct {
    size_t offset;
    fd_rpc_server_context_t *context;
    fd_ux_t *ux;
} fd_rpc_server_ux_frame_buffer_read_t;

typedef struct {
    fd_rpc_server_ux_configuration_t configuration;
    struct k_work send_work;
    fd_rpc_server_ux_frame_buffer_read_t frame_buffer_read;
} fd_rpc_server_ux_t;

fd_rpc_server_ux_t fd_rpc_server_ux;

bool fd_rpc_server_ux_get_display_metadata_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_GetDisplayMetadataRequest *request fd_unused = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_frame_buffer_metadata_t metadata = fd_ux_get_frame_buffer_metadata(ux);
    firefly_ux_v1_GetDisplayMetadataResponse response = {
        .width = metadata.width,
        .height = metadata.height,
    };
    if (metadata.type == fd_ux_frame_buffer_type_gray) {
        response.which_frame_buffer = firefly_ux_v1_GetDisplayMetadataResponse_gray_tag,
        response.frame_buffer.gray.depth = metadata.components.gray.depth;
    } else
    if (metadata.type == fd_ux_frame_buffer_type_color) {
        response.which_frame_buffer = firefly_ux_v1_GetDisplayMetadataResponse_color_tag,
        response.frame_buffer.color.r = metadata.components.color.r;
        response.frame_buffer.color.g = metadata.components.color.g;
        response.frame_buffer.color.b = metadata.components.color.b;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

void fd_rpc_server_ux_frame_buffer_close(void) {
    fd_rpc_server_ux.frame_buffer_read.offset = 0;
    fd_rpc_server_context_free(fd_rpc_server_ux.frame_buffer_read.context);
    fd_rpc_server_ux.frame_buffer_read.ux = NULL;
    fd_rpc_server_ux.frame_buffer_read.context = NULL;
}

bool fd_rpc_server_ux_frame_buffer_read_send(void) {
    if (fd_rpc_server_ux.frame_buffer_read.context == NULL) {
        return true;
    }
    
    fd_rpc_channel_t *channel = fd_rpc_server_context_get_channel(fd_rpc_server_ux.frame_buffer_read.context);
    while (true) {
        size_t free_space = channel->get_free_space();
        if (free_space < fd_rpc_server_ux_frame_buffer_read_min_free_space) {
            break;
        }

        firefly_ux_v1_FrameBufferReadResponse response = {
        };
        fd_ux_frame_buffer_t frame_buffer = fd_ux_get_frame_buffer(fd_rpc_server_ux.frame_buffer_read.ux);
        if (fd_rpc_server_ux.frame_buffer_read.offset >= frame_buffer.size) {
            response.which_message = firefly_ux_v1_FrameBufferReadResponse_completed_tag;
            bool success = fd_rpc_server_send_client_response(fd_rpc_server_ux.frame_buffer_read.context, &response);
            fd_rpc_server_ux_frame_buffer_close();
            return success;
        }
        size_t length = frame_buffer.size - fd_rpc_server_ux.frame_buffer_read.offset;
        if (length > sizeof(response.message.read.data.bytes)) {
            length = sizeof(response.message.read.data.bytes);
        }
        response.which_message = firefly_ux_v1_FrameBufferReadResponse_read_tag;
        memcpy(response.message.read.data.bytes, &frame_buffer.data[fd_rpc_server_ux.frame_buffer_read.offset], length);
        fd_rpc_server_ux.frame_buffer_read.offset += length;
        response.message.read.data.size = length;
        bool success = fd_rpc_server_send_client_response(fd_rpc_server_ux.frame_buffer_read.context, &response);
        if (!success) {
            return false;
        }
    }
    return true;
}

bool fd_rpc_server_ux_frame_buffer_read_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_FrameBufferReadRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_rpc_server_ux.frame_buffer_read.offset = 0;
    fd_rpc_server_ux.frame_buffer_read.ux = ux;
    fd_rpc_server_ux.frame_buffer_read.context = context;
    fd_rpc_server_ux_frame_buffer_read_send();
    return true;
}

bool fd_rpc_server_ux_frame_buffer_read_finalize(fd_rpc_server_context_t *context fd_unused) {
    fd_rpc_server_ux_frame_buffer_close();
    return true;
}

void fd_rpc_server_ux_send_work(struct k_work *work fd_unused) {
    fd_rpc_server_ux_frame_buffer_read_send();
}

void fd_rpc_server_ux_free_space_increased(fd_rpc_server_context_t *context fd_unused) {
    int result = k_work_submit_to_queue(fd_rpc_server_ux.configuration.work_queue, &fd_rpc_server_ux.send_work);
    fd_assert(result >= 0);
}

bool fd_rpc_server_ux_get_screen_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_GetScreenRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    firefly_ux_v1_GetScreenResponse response = {
        .screen_identifier = fd_ux_get_screen(ux),
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_set_screen_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_SetScreenRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_set_screen(ux, request->screen_identifier);
    firefly_ux_v1_SetScreenResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_send_input_events_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_SendInputEventsRequest *request = a_request;
    for (pb_size_t i = 0; i < request->input_events_count; ++i) {
        const firefly_ux_v1_InputEvent *event = &request->input_events[i];
#if 0
        fd_button_event_t button_event = {
            .type = event->type,
            .buttons = event->buttons,
            .timestamp = event->timestamp,
            .duration = event->duration,
        };
        fd_ux_button_event(&button_event);
#endif
    }
    firefly_ux_v1_SendInputEventsResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_update_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_UpdateRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_update(ux);
    firefly_ux_v1_UpdateResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_set_update_enabled_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_SetUpdateEnabledRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_set_update_enabled(ux, request->enabled);
    firefly_ux_v1_SetUpdateEnabledResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_get_update_enabled_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_GetUpdateEnabledRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    firefly_ux_v1_GetUpdateEnabledResponse response = {
        .enabled = fd_ux_get_update_enabled(ux),
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_set_display_configuration_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_SetDisplayConfigurationRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_set_display_configuration(ux, &request->configuration);
    firefly_ux_v1_SetDisplayConfigurationResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_get_display_configuration_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_GetDisplayConfigurationRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    firefly_ux_v1_GetDisplayConfigurationResponse response = {
        .has_configuration = true,
    };
    fd_ux_get_display_configuration(ux, &response.configuration);
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_set_interaction_configuration_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_SetInteractionConfigurationRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    fd_ux_set_interaction_configuration(ux, &request->configuration);
    firefly_ux_v1_SetInteractionConfigurationResponse response = {
    };
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_ux_get_interaction_configuration_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_ux_v1_GetInteractionConfigurationRequest *request = a_request;
    fd_ux_t *ux = fd_ux_get(request->user_interface_identifier);
    firefly_ux_v1_GetInteractionConfigurationResponse response = {
        .has_configuration = true,
    };
    fd_ux_get_interaction_configuration(ux, &response.configuration);
    return fd_rpc_server_send_client_response(context, &response);
}

void fd_rpc_server_ux_initialize(const fd_rpc_server_ux_configuration_t *configuration) {
    memcpy(&fd_rpc_server_ux.configuration, configuration, sizeof(*configuration));
    k_work_init(&fd_rpc_server_ux.send_work, fd_rpc_server_ux_send_work);

    static const fd_rpc_method_server_t get_display_metadata_server = {
        .request = fd_rpc_server_ux_get_display_metadata_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_get_display_metadata, &get_display_metadata_server);

    static const fd_rpc_method_server_t frame_buffer_read_server = {
        .request = fd_rpc_server_ux_frame_buffer_read_request,
        .finalize = fd_rpc_server_ux_frame_buffer_read_finalize,
        .free_space_increased = fd_rpc_server_ux_free_space_increased,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_frame_buffer_read, &frame_buffer_read_server);

    static const fd_rpc_method_server_t get_screen_server = {
        .request = fd_rpc_server_ux_get_screen_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_get_screen, &get_screen_server);

    static const fd_rpc_method_server_t set_screen_server = {
        .request = fd_rpc_server_ux_set_screen_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_set_screen, &set_screen_server);

    static const fd_rpc_method_server_t send_input_events_server = {
        .request = fd_rpc_server_ux_send_input_events_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_send_input_events, &send_input_events_server);

    static const fd_rpc_method_server_t update_server = {
        .request = fd_rpc_server_ux_update_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_update, &update_server);

    static const fd_rpc_method_server_t get_update_enabled_server = {
        .request = fd_rpc_server_ux_get_update_enabled_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_get_update_enabled, &get_update_enabled_server);

    static const fd_rpc_method_server_t set_update_enabled_server = {
        .request = fd_rpc_server_ux_set_update_enabled_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_set_update_enabled, &set_update_enabled_server);

    static const fd_rpc_method_server_t get_display_configuration_server = {
        .request = fd_rpc_server_ux_get_display_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_get_display_configuration, &get_display_configuration_server);

    static const fd_rpc_method_server_t set_display_configuration_server = {
        .request = fd_rpc_server_ux_set_display_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_set_display_configuration, &set_display_configuration_server);

    static const fd_rpc_method_server_t get_interaction_configuration_server = {
        .request = fd_rpc_server_ux_get_interaction_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_get_interaction_configuration, &get_interaction_configuration_server);

    static const fd_rpc_method_server_t set_interaction_configuration_server = {
        .request = fd_rpc_server_ux_set_interaction_configuration_request,
    };
    fd_rpc_set_method_server_association(&fd_rpc_service_ux_set_interaction_configuration, &set_interaction_configuration_server);
}

fd_source_pop()
