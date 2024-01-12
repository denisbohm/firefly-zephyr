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


class Parameters:

    def __init__(self, parameter):
        self.split = parameter == "split"


def protobuf_type_to_nanopb(type):
    return type[1:].replace('.', '_')


def protobuf_name_to_c(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def process_file(
    proto_file: FileDescriptorProto, parameters: Parameters, response: plugin.CodeGeneratorResponse
) -> None:
    if proto_file.options and proto_file.options.HasExtension(options_pb2.c_prefix):
        prefix = proto_file.options.Extensions[options_pb2.c_prefix]
    else:
        prefix = "fd"
    if proto_file.options and proto_file.options.HasExtension(options_pb2.package_id):
        package_id = proto_file.options.Extensions[options_pb2.package_id]
    for service in proto_file.service:
        c_base = f"{prefix}_rpc_service_{protobuf_name_to_c(service.name)}"
        h_content = StringBuilder()
        h_content.string = f'''
#ifndef {c_base}_pb_h
#define {c_base}_pb_h

#include "{prefix}_rpc.h"
'''

        c_content = StringBuilder()
        c_content.string = f'''
#include "{c_base}.pb.h"

#include "{protobuf_name_to_c(service.name)}.pb.h"
'''

        if service.options and service.options.HasExtension(options_pb2.service_id):
            service_id = service.options.Extensions[options_pb2.service_id]
        for method in service.method:
            if method.options and method.options.HasExtension(options_pb2.method_id):
                method_id = method.options.Extensions[options_pb2.method_id]
            if method.options and method.options.HasExtension(options_pb2.access):
                access = method.options.Extensions[options_pb2.access]
            else:
                access = 0
            variable_name = prefix + '_rpc_service_' + protobuf_name_to_c(service.name) + '_' + protobuf_name_to_c(method.name)
            h_content.string += f"""
extern const {prefix}_rpc_method_t {variable_name};
"""
            nanopb_request = protobuf_type_to_nanopb(method.input_type)
            nanopb_response = protobuf_type_to_nanopb(method.output_type)
            c_content.string += f"""
const {prefix}_rpc_method_t {variable_name} = {{
    .package_id = {package_id},
    .service_id = {service_id},
    .method_id = {method_id},
    .request_size = {nanopb_request}_size,
    .request_msgdesc = &{nanopb_request}_msg,
    .response_size = {nanopb_response}_size,
    .response_msgdesc = &{nanopb_response}_msg,
    .server_streaming = {"true" if method.server_streaming else "false"},
    .client_streaming = {"true" if method.client_streaming else "false"},
    .access = {access},
}};
"""

        h_content.string += """
#endif
"""
        h_file = response.file.add()
        if parameters.split:
            h_file.name = f"include/{c_base}.pb.h"
        else:
            h_file.name = f"{c_base}.pb.h"
        h_file.content = h_content.string

        c_file = response.file.add()
        if parameters.split:
            c_file.name = f"src/{c_base}.pb.c"
        else:
            c_file.name = f"{c_base}.pb.c"
        c_file.content = c_content.string


def process(
    request: plugin.CodeGeneratorRequest, parameters: Parameters, response: plugin.CodeGeneratorResponse
) -> None:
    for proto_file in request.proto_file:
        if proto_file.name in request.file_to_generate:
            process_file(proto_file, parameters, response)


def main_plugin():
    data = sys.stdin.buffer.read()
    request = plugin.CodeGeneratorRequest.FromString(data)
    parameters = Parameters(request.parameter)
    response = plugin.CodeGeneratorResponse()
    process(request, parameters, response)
    sys.stdout.buffer.write(response.SerializeToString())


if __name__ == '__main__':
    main_plugin()
