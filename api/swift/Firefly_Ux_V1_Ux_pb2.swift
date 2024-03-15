import SwiftProtobuf
import Foundation

@MainActor
class ux {

    static let Firefly_Ux_V1_Ux_GetDisplayMetadata = RPC.Method(package_id: 9, service_id: 1, method_id: 1, create_request: Firefly_Ux_V1_GetDisplayMetadataRequest.self, create_response: Firefly_Ux_V1_GetDisplayMetadataResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetDisplayMetadata(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_GetDisplayMetadata, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetDisplayMetadata_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_GetDisplayMetadata, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_FrameBufferRead = RPC.Method(package_id: 9, service_id: 1, method_id: 2, create_request: Firefly_Ux_V1_FrameBufferReadRequest.self, create_response: Firefly_Ux_V1_FrameBufferReadResponse.self, server_streaming: true, client_streaming: false, access: 0)

    static func FrameBufferRead_initialize(callback: RPC.ServerStreamingCallback, channel: Transport.Channel? = nil, rpc: RPC? = nil) throws -> RPC.ServerStreamingClient {
        return try RPC.server_streaming_initialize(method: ux.Firefly_Ux_V1_Ux_FrameBufferRead, callback: callback, channel: channel, rpc: rpc)
    }

    static func FrameBufferRead_request(context: RPC.ClientContext, request: SwiftProtobuf.Message?, rpc: RPC? = nil) throws {
        try RPC.server_streaming_request(context: context, request: request, rpc: rpc)
    }

    static func FrameBufferRead_finalize(context: RPC.ClientContext, rpc: RPC? = nil) {
        RPC.server_streaming_finalize(context: context, rpc: rpc)
    }

    static func FrameBufferRead(request: SwiftProtobuf.Message?, callback: RPC.ServerStreamingCallback, channel: Transport.Channel? = nil, rpc: RPC? = nil) throws -> RPC.ServerStreamingClient {
        return try RPC.server_streaming_call(method: ux.Firefly_Ux_V1_Ux_FrameBufferRead, request: request, callback: callback, channel: channel, rpc: rpc)
    }

    static let Firefly_Ux_V1_Ux_GetScreen = RPC.Method(package_id: 9, service_id: 1, method_id: 3, create_request: Firefly_Ux_V1_GetScreenRequest.self, create_response: Firefly_Ux_V1_GetScreenResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetScreen(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_GetScreen, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetScreen_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_GetScreen, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_SetScreen = RPC.Method(package_id: 9, service_id: 1, method_id: 4, create_request: Firefly_Ux_V1_SetScreenRequest.self, create_response: Firefly_Ux_V1_SetScreenResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetScreen(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_SetScreen, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetScreen_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_SetScreen, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_SendInputEvents = RPC.Method(package_id: 9, service_id: 1, method_id: 5, create_request: Firefly_Ux_V1_SendInputEventsRequest.self, create_response: Firefly_Ux_V1_SendInputEventsResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SendInputEvents(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_SendInputEvents, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SendInputEvents_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_SendInputEvents, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_Update = RPC.Method(package_id: 9, service_id: 1, method_id: 6, create_request: Firefly_Ux_V1_UpdateRequest.self, create_response: Firefly_Ux_V1_UpdateResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func Update(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_Update, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func Update_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_Update, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_SetUpdateEnabled = RPC.Method(package_id: 9, service_id: 1, method_id: 7, create_request: Firefly_Ux_V1_SetUpdateEnabledRequest.self, create_response: Firefly_Ux_V1_SetUpdateEnabledResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetUpdateEnabled(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_SetUpdateEnabled, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetUpdateEnabled_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_SetUpdateEnabled, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_GetUpdateEnabled = RPC.Method(package_id: 9, service_id: 1, method_id: 8, create_request: Firefly_Ux_V1_GetUpdateEnabledRequest.self, create_response: Firefly_Ux_V1_GetUpdateEnabledResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetUpdateEnabled(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_GetUpdateEnabled, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetUpdateEnabled_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_GetUpdateEnabled, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_SetDisplayConfiguration = RPC.Method(package_id: 9, service_id: 1, method_id: 9, create_request: Firefly_Ux_V1_SetDisplayConfigurationRequest.self, create_response: Firefly_Ux_V1_SetDisplayConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetDisplayConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_SetDisplayConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetDisplayConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_SetDisplayConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_GetDisplayConfiguration = RPC.Method(package_id: 9, service_id: 1, method_id: 10, create_request: Firefly_Ux_V1_GetDisplayConfigurationRequest.self, create_response: Firefly_Ux_V1_GetDisplayConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetDisplayConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_GetDisplayConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetDisplayConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_GetDisplayConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_GetInteractionConfiguration = RPC.Method(package_id: 9, service_id: 1, method_id: 11, create_request: Firefly_Ux_V1_GetInteractionConfigurationRequest.self, create_response: Firefly_Ux_V1_GetInteractionConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetInteractionConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_GetInteractionConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetInteractionConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_GetInteractionConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Ux_V1_Ux_SetInteractionConfiguration = RPC.Method(package_id: 9, service_id: 1, method_id: 12, create_request: Firefly_Ux_V1_SetInteractionConfigurationRequest.self, create_response: Firefly_Ux_V1_SetInteractionConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetInteractionConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: ux.Firefly_Ux_V1_Ux_SetInteractionConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetInteractionConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: ux.Firefly_Ux_V1_Ux_SetInteractionConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

}