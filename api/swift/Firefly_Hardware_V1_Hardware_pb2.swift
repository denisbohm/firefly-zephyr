import SwiftProtobuf
import Foundation

@MainActor
class Hardware {

    static let Firefly_Hardware_V1_Hardware_GetDeviceIdentifier = RPC.Method(package_id: 8, service_id: 1, method_id: 1, create_request: Firefly_Hardware_V1_GetDeviceIdentifierRequest.self, create_response: Firefly_Hardware_V1_GetDeviceIdentifierResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetDeviceIdentifier(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_GetDeviceIdentifier, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetDeviceIdentifier_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_GetDeviceIdentifier, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Hardware_V1_Hardware_ConfigurePort = RPC.Method(package_id: 8, service_id: 1, method_id: 2, create_request: Firefly_Hardware_V1_ConfigurePortRequest.self, create_response: Firefly_Hardware_V1_ConfigurePortResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func ConfigurePort(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_ConfigurePort, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func ConfigurePort_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_ConfigurePort, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Hardware_V1_Hardware_GetPort = RPC.Method(package_id: 8, service_id: 1, method_id: 3, create_request: Firefly_Hardware_V1_GetPortRequest.self, create_response: Firefly_Hardware_V1_GetPortResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func GetPort(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_GetPort, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func GetPort_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_GetPort, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Hardware_V1_Hardware_SetPort = RPC.Method(package_id: 8, service_id: 1, method_id: 4, create_request: Firefly_Hardware_V1_SetPortRequest.self, create_response: Firefly_Hardware_V1_SetPortResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SetPort(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_SetPort, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SetPort_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_SetPort, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Hardware_V1_Hardware_SpiTransceive = RPC.Method(package_id: 8, service_id: 1, method_id: 5, create_request: Firefly_Hardware_V1_SpiTransceiveRequest.self, create_response: Firefly_Hardware_V1_SpiTransceiveResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func SpiTransceive(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_SpiTransceive, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func SpiTransceive_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_SpiTransceive, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

    static let Firefly_Hardware_V1_Hardware_I2CTransfer = RPC.Method(package_id: 8, service_id: 1, method_id: 6, create_request: Firefly_Hardware_V1_I2cTransferRequest.self, create_response: Firefly_Hardware_V1_I2cTransferResponse.self, server_streaming: false, client_streaming: false, access: 0)

    static func I2CTransfer(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {
        return try await RPC.unary_call(method: Hardware.Firefly_Hardware_V1_Hardware_I2CTransfer, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }

    static func I2CTransfer_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {
        return try RPC.unary_call_async(method: Hardware.Firefly_Hardware_V1_Hardware_I2CTransfer, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }

}