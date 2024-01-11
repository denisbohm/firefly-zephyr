from google.protobuf.internal.decoder import _DecodeVarint
from google.protobuf.internal.encoder import _EncodeVarint

from cobs import cobs

import crcmod

import serial
import serial.tools.list_ports
from serial.serialutil import SerialException

from enum import Enum
import queue
import select
import socket
import threading
import traceback


class Transport:

    class DecodeError(Exception):

        def __init__(self, message):
            super().__init__(message)

    class Handler:

        def handle(self, channel, data):
            pass

    class Channel:

        def __init__(self):
            if Transport.default_channel is None:
                Transport.default_channel = self

        def packet_write(self, binary):
            pass

        def start_encryption(self, key):
            self.key = key
            self.is_secure = True

    class StreamChannel(Channel):

        def __init__(self, transport):
            super().__init__()
            self.transport = transport
            self.buffer = bytearray()
            self.data = bytes()

        def write(self, data):
            pass

        def packet_write(self, packet):
            self.write(cobs.encode(packet) + bytes([Transport.Packet.delimiter]))

        def packet_read(self, data):
            try:
                packet = cobs.decode(data)
                self.transport.receive(self, packet)
            except Exception as exception:
                print(f"RPC Stream Channel: {str(exception)}")
                print(traceback.format_exc())

        def read(self, data):
            for byte in data:
                if byte != Transport.Packet.delimiter:
                    self.buffer.append(byte)
                    continue
                if len(self.buffer) == 0:
                    continue
                buffer = self.buffer
                self.buffer = bytearray()
                self.packet_read(buffer)

    class Packet:
    
        delimiter = 0

        class Type(Enum):
            rpc_server_initialize = 0
            rpc_server_request = 1
            rpc_server_finalize = 2
            rpc_client_response = 3
            rpc_client_finalize = 4

            count = 5

    default_channel = None

    def __init__(self):
        self.crc16 = crcmod.mkCrcFun(0x11021, rev=False, initCrc=0x0000, xorOut=0x0000)
        self.handlers = {}

    def set_handler(self, type, handler):
        self.handlers[type.value] = handler

    def write(self, channel, type, data):
        type_data = bytearray()
        _EncodeVarint(type_data.extend, type.value)
        length = len(type_data) + len(data)
        length_data = bytearray([length & 0xff, (length >> 8) & 0xff])
        crc16 = self.crc16(length_data + type_data + data)
        crc16_data = bytearray([crc16 & 0xff, (crc16 >> 8) & 0xff])
        packet = crc16_data + length_data + type_data + data
        channel.packet_write(packet)

    def receive(self, channel, data):
        crc16_size = 2
        length_size = 2
        header_size = crc16_size + length_size
        size = len(data)
        if size < header_size:
            raise Transport.DecodeError("missing header")
        crc16 = (data[1] << 8) | data[0]
        length = (data[3] << 8) | data[2]
        length_actual = size - header_size
        if length_actual != length:
            raise Transport.DecodeError("length mismatch")
        crc16_actual = self.crc16(data[2:])
        if crc16_actual != crc16:
            raise Transport.DecodeError("crc mismatch")
        (type, offset) = _DecodeVarint(data[4:], 0)
        if type >= Transport.Packet.Type.count.value:
            raise Transport.DecodeError("unknown transport packet type")
        if type not in self.handlers:
            raise Transport.DecodeError("transport packet type handler not found")
        handler = self.handlers[type]
        handler(channel, data[4 + offset:])


class SerialChannel(Transport.StreamChannel):

    @staticmethod
    def find_serial_port(vid=0x2fe3, pid=0x0001):
        for info in serial.tools.list_ports.comports():
            if info.vid == vid and info.pid == pid:
                return info.device
        return None

    def __init__(self, transport):
        super().__init__(transport)
        self.thread = None
        self.port = None
        self.serial_port = None
        self.run = False

    def write(self, data):
        self.serial_port.write(data)

    def run_loop(self):
        try:
            while True:
                data = self.serial_port.read(1024)
                if len(data) > 0:
                    self.read(data)
                if not self.run:
                    break
        except SerialException:
            pass

    def run_loop_in_thread(self):
        self.run = True
        self.thread = threading.Thread(target=self.run_loop, args=())
        self.thread.start()

    def close(self):
        self.run = False
        self.thread.join()
        self.serial_port.close()

    def run_client(self):
        self.port = SerialChannel.find_serial_port(vid=0x2fe3, pid=0x0100)
        if not self.port:
            raise Transport.DecodeError("USB serial port not found")
        self.serial_port = serial.Serial(
            port=self.port,
            baudrate=9600,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=0.250
        )
        print(f"client using {self.port}")
        self.run_loop_in_thread()


class SocketChannel(Transport.StreamChannel):

    def __init__(self, transport, host="127.0.0.1", port=65432):
        super().__init__(transport)
        self.host = host
        self.port = port
        self.server_socket = None
        self.socket = None
        self.thread = None
        self.send_queue = queue.Queue()
        self.send_queue_receive_socket, self.send_queue_send_socket = socket.socketpair()

    def write(self, data):
        self.send_queue.put(data)
        self.send_queue_send_socket.send(b"\x00")

    def run_loop(self):
        while True:
            ready_sockets, _, _ = select.select([self.socket, self.send_queue_receive_socket], [], [])
            for ready_socket in ready_sockets:
                if ready_socket is self.socket:
                    data = self.socket.recv(1024)
                    self.read(data)
                if ready_socket is self.send_queue_receive_socket:
                    self.send_queue_receive_socket.recv(1)
                    self.socket.sendall(self.send_queue.get())

    def run_loop_in_thread(self):
        self.thread = threading.Thread(target=self.run_loop, args=())
        self.thread.start()

    def run_server(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen()
        self.socket, address = self.server_socket.accept()
        print(f"server accept from {address}")
        self.run_loop_in_thread()

    def run_client(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))
        print(f"client connect to {self.host}:{self.port}")
        self.run_loop_in_thread()


class RPC:

    class DecodeError(Exception):

        def __init__(self, message):
            super().__init__(message)

    class TimeoutError(Exception):

        def __init__(self, message):
            super().__init__(message)

    class Server:

        def __init__(self):
            self.context = None

        def initialize(self, context):
            self.context = context

        def request(self, context, request):
            pass

        def finalize(self, context):
            self.context = None

    class Client:

        def __init__(self):
            self.context = None

        def response(self, context, response):
            pass
        
        def finalize(self, context):
            self.context = None

    class UnaryClient(Client):

        def __init__(self):
            super().__init__()
            self.queue = queue.Queue()

        def response(self, context, response):
            self.queue.put(response)

    class ServerStreamingCallback:

        def __init__(self):
            pass

        def response(self, response):
            pass

        def finalize(self):
            pass

    class ServerStreamingClient(Client):

        def __init__(self, rpc, callback):
            super().__init__()
            self.rpc = rpc
            self.callback = callback

        def close(self):
            self.rpc.client_send_server_finalize(self.context)

        def response(self, context, response):
            self.callback.response(response)

        def finalize(self, context, response):
            self.callback.finalize()

    class Access(Enum):
        secure =0
        open = 1
    
    class Method:

        def __init__(self, package_id, service_id, method_id, create_request, create_response, server_streaming, client_streaming, access):
            self.package_id = package_id
            self.service_id = service_id
            self.method_id = method_id
            self.create_request = create_request
            self.create_response = create_response
            self.server_streaming = server_streaming
            self.client_streaming = client_streaming
            self.access = access

    class Call:

        def __init__(self, package_id, service_id, method_id, invocation):
            self.package_id = package_id
            self.service_id = service_id
            self.method_id = method_id
            self.invocation = invocation

    class MethodKey:

        def __init__(self, package_id, service_id, method_id):
            self.package_id = package_id
            self.service_id = service_id
            self.method_id = method_id

        def __hash__(self):
            return hash((self.package_id, self.service_id, self.method_id))

        def __eq__(self, other):
            return (self.package_id, self.service_id, self.method_id) ==\
                   (other.package_id, other.service_id, other.method_id)

    class CommonContext:

        def __init__(self, channel, invocation, method):
            self.channel = channel
            self.invocation = invocation
            self.method = method

    class ServerContext(CommonContext):

        def __init__(self, channel, invocation, method, server):
            super().__init__(channel, invocation, method)
            self.server = server
            
    class ClientContext(CommonContext):

        def __init__(self, channel, invocation, method, client):
            super().__init__(channel, invocation, method)
            self.client = client
            
    class MethodServerAssociation:
        
        def __init__(self, method, server):
            self.method = method
            self.server = server

    default_instance = None

    def __init__(self, transport):
        if RPC.default_instance is None:
            RPC.default_instance = self

        self.transport = transport
        self.method_server_associations = {}
        self.server_contexts = []
        self.client_contexts = []
        self.next_invocation = 0
        self.unary_timeout = 2.0

        transport.set_handler(Transport.Packet.Type.rpc_server_initialize, self.transport_server_initialize)
        transport.set_handler(Transport.Packet.Type.rpc_server_request, self.transport_server_request)
        transport.set_handler(Transport.Packet.Type.rpc_server_finalize, self.transport_server_finalize)

        transport.set_handler(Transport.Packet.Type.rpc_client_response, self.transport_client_response)
        transport.set_handler(Transport.Packet.Type.rpc_client_finalize, self.transport_client_finalize)

    def set_method_server_association(self, method, server):
        key = RPC.MethodKey(method.package_id, method.service_id, method.method_id)
        self.method_server_associations[key] = RPC.MethodServerAssociation(method, server)

    def get_method_server_association(self, call):
        key = RPC.MethodKey(call.package_id, call.service_id, call.method_id)
        if key not in self.method_server_associations:
            raise RPC.DecodeError("server not found")
        return self.method_server_associations[key]
    
    def server_context_allocate(self, channel, invocation, method, server):
        context = RPC.ServerContext(channel, invocation, method, server)
        self.server_contexts.append(context)
        return context

    def server_context_get(self, channel, call):
        for context in self.server_contexts:
            if context.channel != channel:
                continue
            method = context.method
            if method.package_id != call.package_id:
                continue
            if method.service_id != call.service_id:
                continue
            if method.method_id != call.method_id:
                continue
            if context.invocation != call.invocation:
                continue
            return context
        return None

    def server_context_free(self, context):
        self.server_contexts.remove(context)

    def client_context_allocate(self, channel, invocation, method, client):
        context = RPC.ClientContext(channel, invocation, method, client)
        self.client_contexts.append(context)
        return context

    def client_context_get(self, channel, call):
        for context in self.client_contexts:
            if context.channel != channel:
                continue
            method = context.method
            if method.package_id != call.package_id:
                continue
            if method.service_id != call.service_id:
                continue
            if method.method_id != call.method_id:
                continue
            if context.invocation != call.invocation:
                continue
            return context
        return None

    def client_context_free(self, context):
        self.client_contexts.remove(context)

    def decode_call(self, data, position=0):
        (package_id, position) = _DecodeVarint(data, position)
        (service_id, position) = _DecodeVarint(data, position)
        (method_id, position) = _DecodeVarint(data, position)
        (invocation, position) = _DecodeVarint(data, position)
        return RPC.Call(package_id, service_id, method_id, invocation), position

    def transport_server_initialize(self, channel, data):
        (call, position) = self.decode_call(data, 0)
        association = self.get_method_server_association(call)
        context = self.server_context_allocate(channel, call.invocation, association.method, association.server)
        try:
            context.server.initialize(context)
        except Exception:
            self.server_context_free(context)
            raise

    def transport_server_request(self, channel, data):
        (call, position) = self.decode_call(data, 0)

        association = self.get_method_server_association(call)
        if association is None:
            return

        if association.method.client_streaming:
            context = self.server_context_get(channel, call)
        else:
            context = self.server_context_allocate(channel, call.invocation, association.method, association.server)
        if context is None:
            return

        request = context.method.create_request()
        request.ParseFromString(data[position:])
        try:
            context.server.request(context, request)
        except Exception:
            if (not association.method.client_streaming) and (not association.method.server_streaming):
                self.server_context_free(context)
            raise

    def transport_server_finalize(self, channel, data):
        (call, position) = self.decode_call(data, 0)
        context = self.server_context_get(channel, call)
        if context is None:
            return
        self.server_context_free(context)
        context.server.finalize(context)

    def transport_client_response(self, channel, data):
        (call, position) = self.decode_call(data, 0)
        context = self.client_context_get(channel, call)
        if context is None:
            return
        response = context.method.create_response()
        response.ParseFromString(data[position:])
        context.client.response(context, response)

    def transport_client_finalize(self, channel, data):
        (call, position) = self.decode_call(data, 0)
        context = self.client_context_get(channel, call)
        if context is None:
            return
        self.client_context_free(context)
        context.client.finalize(context)

    def send(self, context, type, message):
        data = bytearray()
        method = context.method
        _EncodeVarint(data.extend, method.package_id)
        _EncodeVarint(data.extend, method.service_id)
        _EncodeVarint(data.extend, method.method_id)
        _EncodeVarint(data.extend, context.invocation)
        if message is not None:
            data.extend(message.SerializeToString())
        self.transport.write(context.channel, type, data)

    def server_send_client_response(self, context, response):
        self.send(context, Transport.Packet.Type.rpc_client_response, response)

    def server_send_client_finalize(self, context):
        self.send(context, Transport.Packet.Type.rpc_client_finalize, None)

    def client_send_server_initialize(self, context):
        self.send(context, Transport.Packet.Type.rpc_server_initialize, None)

    def client_send_server_request(self, context, request):
        self.send(context, Transport.Packet.Type.rpc_server_request, request)

    def client_send_server_finalize(self, context):
        self.send(context, Transport.Packet.Type.rpc_server_finalize, None)

    def get_unique_invocation(self):
        self.next_invocation += 1
        return self.next_invocation

    @staticmethod
    def unary_call(method, request, channel=None, rpc=None, timeout=None):
        if rpc is None:
            rpc = RPC.default_instance
        if channel is None:
            channel = Transport.default_channel
        if timeout is None:
            timeout = rpc.unary_timeout
        client = RPC.UnaryClient()
        invocation = rpc.get_unique_invocation()
        context = rpc.client_context_allocate(channel, invocation, method, client)
        response = None
        try:
            rpc.client_send_server_request(context, request)
            response = client.queue.get(block=True, timeout=timeout)
        except queue.Empty:
            pass
        rpc.client_context_free(context)
        if response is None:
            raise RPC.TimeoutError("unary call timeout")
        return response

    @staticmethod
    def server_streaming_initialize(method, callback, channel=None, rpc=None):
        if rpc is None:
            rpc = RPC.default_instance
        if channel is None:
            channel = Transport.default_channel
        client = RPC.ServerStreamingClient(rpc, callback)
        invocation = rpc.get_unique_invocation()
        client.context = rpc.client_context_allocate(channel, invocation, method, client)
        rpc.client_send_server_initialize(client.context)
        return client

    @staticmethod
    def server_streaming_request(context, request, rpc=None):
        if rpc is None:
            rpc = RPC.default_instance
        rpc.client_send_server_request(context, request)

    @staticmethod
    def server_streaming_finalize(context, rpc=None):
        if rpc is None:
            rpc = RPC.default_instance
        rpc.client_send_server_finalize(context)
        rpc.client_context_free(context)

    @staticmethod
    def server_streaming_call(method, request, callback, channel=None, rpc=None):
        if rpc is None:
            rpc = RPC.default_instance
        if channel is None:
            channel = Transport.default_channel
        client = RPC.ServerStreamingClient(rpc, callback)
        invocation = rpc.get_unique_invocation()
        client.context = rpc.client_context_allocate(channel, invocation, method, client)
        rpc.client_send_server_request(client.context, request)
        return client
