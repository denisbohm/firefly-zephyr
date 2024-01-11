import SwiftProtobuf
import Foundation

@MainActor
class RTC {

    static let Firefly_Rtc_V1_Rtc_GetTime = RPC.Method(package_id: 10, service_id: 1, method_id: 1, create_request: Firefly_Rtc_V1_GetTimeRequest.self, create_response: Firefly_Rtc_V1_GetTimeResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetTime(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: RTC.Firefly_Rtc_V1_Rtc_GetTime, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetTime_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: RTC.Firefly_Rtc_V1_Rtc_GetTime, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Rtc_V1_Rtc_SetTime = RPC.Method(package_id: 10, service_id: 1, method_id: 2, create_request: Firefly_Rtc_V1_SetTimeRequest.self, create_response: Firefly_Rtc_V1_SetTimeResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetTime(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: RTC.Firefly_Rtc_V1_Rtc_SetTime, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetTime_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: RTC.Firefly_Rtc_V1_Rtc_SetTime, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Rtc_V1_Rtc_GetConfiguration = RPC.Method(package_id: 10, service_id: 1, method_id: 3, create_request: Firefly_Rtc_V1_GetConfigurationRequest.self, create_response: Firefly_Rtc_V1_GetConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: RTC.Firefly_Rtc_V1_Rtc_GetConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: RTC.Firefly_Rtc_V1_Rtc_GetConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Rtc_V1_Rtc_SetConfiguration = RPC.Method(package_id: 10, service_id: 1, method_id: 4, create_request: Firefly_Rtc_V1_SetConfigurationRequest.self, create_response: Firefly_Rtc_V1_SetConfigurationResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetConfiguration(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: RTC.Firefly_Rtc_V1_Rtc_SetConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetConfiguration_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: RTC.Firefly_Rtc_V1_Rtc_SetConfiguration, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

}