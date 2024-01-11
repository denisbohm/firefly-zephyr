#!/usr/bin/env python3

import os

if not os.getenv("PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION"):
    os.putenv("PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION", "python")
    os.environ["PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION"] = "python"

import options_pb2

from google.protobuf.descriptor_pb2 import FileDescriptorProto
import google.protobuf.compiler.plugin_pb2 as plugin

import re
import sys


class StringBuilder:

    def __init__(self):
        self.string = ""


def protobuf_type_to_swift_class(type):
    return type.rsplit('.')[-1]


def protobuf_name_to_swift(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).title().replace('_', '').replace('.', '_')


def swift_bool(value):
    return 'true' if value else 'false'


def process_file(
    proto_file: FileDescriptorProto, response: plugin.CodeGeneratorResponse
) -> None:
    swift_package_name = protobuf_name_to_swift(proto_file.package)
    if proto_file.options and proto_file.options.HasExtension(options_pb2.package_id):
        package_id = proto_file.options.Extensions[options_pb2.package_id]
    for service in proto_file.service:
        swift_service_name = protobuf_name_to_swift(service.name)
        swift_base = f"{swift_package_name}_{swift_service_name}"

        swift_content = StringBuilder()
        swift_content.string = f"""import SwiftProtobuf
import Foundation

@MainActor
class {service.name} {{
"""

        if service.options and service.options.HasExtension(options_pb2.service_id):
            service_id = service.options.Extensions[options_pb2.service_id]
        for method in service.method:
            if method.options and method.options.HasExtension(options_pb2.method_id):
                method_id = method.options.Extensions[options_pb2.method_id]
            if method.options and method.options.HasExtension(options_pb2.access):
                access = method.options.Extensions[options_pb2.access]
            else:
                access = 0
            swift_method_name = protobuf_name_to_swift(method.name)
            variable_name = swift_package_name + '_' + swift_service_name + '_' + swift_method_name
            request_class = swift_package_name + '_' + protobuf_type_to_swift_class(method.input_type)
            response_class = swift_package_name + '_' + protobuf_type_to_swift_class(method.output_type)
            swift_content.string += f"""
    static let {variable_name} = RPC.Method(package_id: {package_id}, service_id: {service_id}, method_id: {method_id}, create_request: {request_class}.self, create_response: {response_class}.self, server_streaming: {swift_bool(method.server_streaming)}, client_streaming: {swift_bool(method.client_streaming)}, access: {access})
"""
            if not method.server_streaming and not method.client_streaming:
                swift_content.string += f"""
    static func {protobuf_name_to_swift(method.name)}(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil) async throws -> SwiftProtobuf.Message? {{
        return try await RPC.unary_call(method: {service.name}.{variable_name}, request: request, channel: channel, rpc: rpc, timeout: timeout)
    }}

    static func {protobuf_name_to_swift(method.name)}_async(request: SwiftProtobuf.Message?, channel: Transport.Channel? = nil, rpc: RPC? = nil, timeout: Double? = nil, handler: @escaping (SwiftProtobuf.Message?) -> ()) throws {{
        return try RPC.unary_call_async(method: {service.name}.{variable_name}, request: request, channel: channel, rpc: rpc, timeout: timeout, handler: handler)
    }}
"""
            if method.server_streaming or method.client_streaming:
                swift_content.string += f"""
    static func {protobuf_name_to_swift(method.name)}_initialize(callback: RPC.ServerStreamingCallback, channel: Transport.Channel? = nil, rpc: RPC? = nil) throws -> RPC.ServerStreamingClient {{
        return try RPC.server_streaming_initialize(method: {service.name}.{variable_name}, callback: callback, channel: channel, rpc: rpc)
    }}

    static func {protobuf_name_to_swift(method.name)}_request(context: RPC.ClientContext, request: SwiftProtobuf.Message?, rpc: RPC? = nil) throws {{
        try RPC.server_streaming_request(context: context, request: request, rpc: rpc)
    }}

    static func {protobuf_name_to_swift(method.name)}_finalize(context: RPC.ClientContext, rpc: RPC? = nil) {{
        RPC.server_streaming_finalize(context: context, rpc: rpc)
    }}
"""
            if method.server_streaming and not method.client_streaming:
                swift_content.string += f"""
    static func {protobuf_name_to_swift(method.name)}(request: SwiftProtobuf.Message?, callback: RPC.ServerStreamingCallback, channel: Transport.Channel? = nil, rpc: RPC? = nil) throws -> RPC.ServerStreamingClient {{
        return try RPC.server_streaming_call(method: {service.name}.{variable_name}, request: request, callback: callback, channel: channel, rpc: rpc)
    }}
"""

        swift_content.string += """
}"""

        swift_file = response.file.add()
        swift_file.name = f'{swift_base}_pb2.swift'
        swift_file.content = swift_content.string


def process(
    request: plugin.CodeGeneratorRequest, response: plugin.CodeGeneratorResponse
) -> None:
    for proto_file in request.proto_file:
        if proto_file.name in request.file_to_generate:
            process_file(proto_file, response)


def main_plugin():
    data = sys.stdin.buffer.read()
    request = plugin.CodeGeneratorRequest.FromString(data)
    response = plugin.CodeGeneratorResponse()
    process(request, response)
    sys.stdout.buffer.write(response.SerializeToString())


if __name__ == '__main__':
    main_plugin()
