import ux_pb2

from rpc import RPC


class ux:

    ux_get_display_metadata = RPC.Method(9, 1, 1, ux_pb2.GetDisplayMetadataRequest, ux_pb2.GetDisplayMetadataResponse, False, False, 0)

    @staticmethod
    def get_display_metadata(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_get_display_metadata, request, channel, rpc, timeout)

    ux_frame_buffer_read = RPC.Method(9, 1, 2, ux_pb2.FrameBufferReadRequest, ux_pb2.FrameBufferReadResponse, True, False, 0)

    @staticmethod
    def frame_buffer_read_initialize(callback, channel=None, rpc=None):
        return RPC.server_streaming_initialize(ux.ux_frame_buffer_read, callback, channel, rpc)

    @staticmethod
    def frame_buffer_read_request(context, request, rpc=None):
        return RPC.server_streaming_request(context, request, rpc)

    @staticmethod
    def frame_buffer_read_finalize(context, rpc=None):
        return RPC.server_streaming_finalize(context, rpc)

    @staticmethod
    def frame_buffer_read(request, callback, channel=None, rpc=None):
        return RPC.server_streaming_call(ux.ux_frame_buffer_read, request, callback, channel, rpc)

    ux_get_screen = RPC.Method(9, 1, 3, ux_pb2.GetScreenRequest, ux_pb2.GetScreenResponse, False, False, 0)

    @staticmethod
    def get_screen(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_get_screen, request, channel, rpc, timeout)

    ux_set_screen = RPC.Method(9, 1, 4, ux_pb2.SetScreenRequest, ux_pb2.SetScreenResponse, False, False, 0)

    @staticmethod
    def set_screen(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_set_screen, request, channel, rpc, timeout)

    ux_send_input_events = RPC.Method(9, 1, 5, ux_pb2.SendInputEventsRequest, ux_pb2.SendInputEventsResponse, False, False, 0)

    @staticmethod
    def send_input_events(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_send_input_events, request, channel, rpc, timeout)

    ux_update = RPC.Method(9, 1, 6, ux_pb2.UpdateRequest, ux_pb2.UpdateResponse, False, False, 0)

    @staticmethod
    def update(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_update, request, channel, rpc, timeout)

    ux_set_update_enabled = RPC.Method(9, 1, 7, ux_pb2.SetUpdateEnabledRequest, ux_pb2.SetUpdateEnabledResponse, False, False, 0)

    @staticmethod
    def set_update_enabled(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_set_update_enabled, request, channel, rpc, timeout)

    ux_get_update_enabled = RPC.Method(9, 1, 8, ux_pb2.GetUpdateEnabledRequest, ux_pb2.GetUpdateEnabledResponse, False, False, 0)

    @staticmethod
    def get_update_enabled(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_get_update_enabled, request, channel, rpc, timeout)

    ux_set_display_configuration = RPC.Method(9, 1, 9, ux_pb2.SetDisplayConfigurationRequest, ux_pb2.SetDisplayConfigurationResponse, False, False, 0)

    @staticmethod
    def set_display_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_set_display_configuration, request, channel, rpc, timeout)

    ux_get_display_configuration = RPC.Method(9, 1, 10, ux_pb2.GetDisplayConfigurationRequest, ux_pb2.GetDisplayConfigurationResponse, False, False, 0)

    @staticmethod
    def get_display_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_get_display_configuration, request, channel, rpc, timeout)

    ux_get_interaction_configuration = RPC.Method(9, 1, 11, ux_pb2.GetInteractionConfigurationRequest, ux_pb2.GetInteractionConfigurationResponse, False, False, 0)

    @staticmethod
    def get_interaction_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_get_interaction_configuration, request, channel, rpc, timeout)

    ux_set_interaction_configuration = RPC.Method(9, 1, 12, ux_pb2.SetInteractionConfigurationRequest, ux_pb2.SetInteractionConfigurationResponse, False, False, 0)

    @staticmethod
    def set_interaction_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(ux.ux_set_interaction_configuration, request, channel, rpc, timeout)
