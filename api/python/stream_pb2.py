# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: stream.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x0cstream.proto\x12\x11\x66irefly.stream.v1\"k\n\x05Value\x12\x14\n\nbool_value\x18\x01 \x01(\x08H\x00\x12\x16\n\x0cuint32_value\x18\x02 \x01(\rH\x00\x12\x15\n\x0bint32_value\x18\x03 \x01(\x05H\x00\x12\x15\n\x0b\x66loat_value\x18\x04 \x01(\x02H\x00\x42\x06\n\x04kind\"\xac\x02\n\x17\x43onnectionConfiguration\x12\x35\n\x13maximum_flow_window\x18\x01 \x01(\x0b\x32\x18.firefly.stream.v1.Value\x12;\n\x19maximum_transmission_unit\x18\x02 \x01(\x0b\x32\x18.firefly.stream.v1.Value\x12\x34\n\x12receive_keep_alive\x18\x03 \x01(\x0b\x32\x18.firefly.stream.v1.Value\x12\x31\n\x0fsend_keep_alive\x18\x04 \x01(\x0b\x32\x18.firefly.stream.v1.Value\x12\x34\n\x12send_retry_timeout\x18\x05 \x01(\x0b\x32\x18.firefly.stream.v1.Valueb\x06proto3')



_VALUE = DESCRIPTOR.message_types_by_name['Value']
_CONNECTIONCONFIGURATION = DESCRIPTOR.message_types_by_name['ConnectionConfiguration']
Value = _reflection.GeneratedProtocolMessageType('Value', (_message.Message,), {
  'DESCRIPTOR' : _VALUE,
  '__module__' : 'stream_pb2'
  # @@protoc_insertion_point(class_scope:firefly.stream.v1.Value)
  })
_sym_db.RegisterMessage(Value)

ConnectionConfiguration = _reflection.GeneratedProtocolMessageType('ConnectionConfiguration', (_message.Message,), {
  'DESCRIPTOR' : _CONNECTIONCONFIGURATION,
  '__module__' : 'stream_pb2'
  # @@protoc_insertion_point(class_scope:firefly.stream.v1.ConnectionConfiguration)
  })
_sym_db.RegisterMessage(ConnectionConfiguration)

if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  _VALUE._serialized_start=35
  _VALUE._serialized_end=142
  _CONNECTIONCONFIGURATION._serialized_start=145
  _CONNECTIONCONFIGURATION._serialized_end=445
# @@protoc_insertion_point(module_scope)