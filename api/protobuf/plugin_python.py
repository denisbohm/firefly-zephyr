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


def protobuf_type_to_python_class(type):
    return type.rsplit('.')[-1]


def protobuf_name_to_python(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower().replace('.', '_')

def remove_suffix(input_string, suffix):
    if suffix and input_string.endswith(suffix):
        return input_string[:-len(suffix)]
    return input_string

def process_file(
    proto_file: FileDescriptorProto, response: plugin.CodeGeneratorResponse
) -> None:
    python_package_name = protobuf_name_to_python(proto_file.package)
    py_name = remove_suffix(proto_file.name, '.proto') + '_pb2'
    if proto_file.options and proto_file.options.HasExtension(options_pb2.package_id):
        package_id = proto_file.options.Extensions[options_pb2.package_id]
    for service in proto_file.service:
        py_service_name = protobuf_name_to_python(service.name)
        py_base = f"{python_package_name}_{py_service_name}"

        py_content = StringBuilder()
        py_content.string = f"""import {py_service_name}_pb2

from rpc import RPC


class {service.name}:
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
            py_method_name = protobuf_name_to_python(method.name)
            variable_name = py_service_name + '_' + py_method_name
            request_class = py_name + '.' + protobuf_type_to_python_class(method.input_type)
            response_class = py_name + '.' + protobuf_type_to_python_class(method.output_type)
            py_content.string += f"""
    {variable_name} = RPC.Method({package_id}, {service_id}, {method_id}, {request_class}, {response_class}, {method.server_streaming}, {method.client_streaming}, {access})
"""
            if not method.server_streaming and not method.client_streaming:
                py_content.string += f"""
    @staticmethod
    def {protobuf_name_to_python(method.name)}(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call({service.name}.{variable_name}, request, channel, rpc, timeout)
"""
            if method.server_streaming or method.client_streaming:
                py_content.string += f"""
    @staticmethod
    def {protobuf_name_to_python(method.name)}_initialize(callback, channel=None, rpc=None):
        return RPC.server_streaming_initialize({service.name}.{variable_name}, callback, channel, rpc)

    @staticmethod
    def {protobuf_name_to_python(method.name)}_request(context, request, rpc=None):
        return RPC.server_streaming_request(context, request, rpc)

    @staticmethod
    def {protobuf_name_to_python(method.name)}_finalize(context, rpc=None):
        return RPC.server_streaming_finalize(context, rpc)
"""
            if method.server_streaming and not method.client_streaming:
                py_content.string += f"""
    @staticmethod
    def {protobuf_name_to_python(method.name)}(request, callback, channel=None, rpc=None):
        return RPC.server_streaming_call({service.name}.{variable_name}, request, callback, channel, rpc)
"""

        py_file = response.file.add()
        py_file.name = f'{py_base}_pb2.py'
        py_file.content = py_content.string


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
