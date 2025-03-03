import stream_pb2

from google.protobuf.internal.decoder import _DecodeVarint
from google.protobuf.internal.encoder import _EncodeVarint

from cobs import cobs

import crcmod

import serial
import serial.tools.list_ports
from serial.serialutil import SerialException

from enum import Enum
import queue
import random
import select
import socket
import struct
import threading
import time
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

        def close(self):
            if Transport.default_channel is self:
                Transport.default_channel = None

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

        def close(self):
            super().close()
            self.buffer = bytearray()

        def write(self, data):
            pass

        def packet_write(self, packet):
            self.write(cobs.encode(packet) + bytes([Transport.Packet.delimiter]))

        def packet_read(self, data):
            try:
                packet = cobs.decode(data)
                self.transport.receive(self, packet)
            except Exception as exception:
                RPC.log(f"{time.time()}: RPC Stream Channel: {str(exception)}, data: {data}")
                RPC.log(traceback.format_exc())

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
                data = self.serial_port.read(1)
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
        self.thread = None
        super().close()
        self.serial_port.close()
        self.serial_port = None
        self.port = None

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

    def __init__(self, transport, host, port, family=socket.AF_INET):
        super().__init__(transport)
        self.host = host
        self.port = port
        self.family = family
        self.server_socket = None
        self.socket = None
        self.run = False
        self.thread = None
        self.send_queue = queue.Queue()
        self.send_queue_receive_socket, self.send_queue_send_socket = socket.socketpair()

    def write(self, data):
        RPC.log(f"{time.time()}: socket write {len(data)}")
        self.send_queue.put(data)
        self.send_queue_send_socket.send(b"\x00")

    def run_loop(self):
        while self.run:
            ready_sockets, _, _ = select.select([self.socket, self.send_queue_receive_socket], [], [], 1.0)
            RPC.log(f"{time.time()}: ready sockets {repr(ready_sockets)}")
            for ready_socket in ready_sockets:
                if ready_socket is self.socket:
                    RPC.log(f"{time.time()}: socket recv")
                    data = self.socket.recv(1024)
                    RPC.log(f"{time.time()}: socket read {len(data)}")
                    self.read(data)
                if ready_socket is self.send_queue_receive_socket:
                    self.send_queue_receive_socket.recv(1)
                    data = self.send_queue.get()
                    RPC.log(f"{time.time()}: socket sendall {len(data)}")
                    self.socket.sendall(data)
                    RPC.log(f"{time.time()}: socket sendall done")

    def run_loop_in_thread(self):
        self.run = True
        self.thread = threading.Thread(target=self.run_loop, args=())
        self.thread.start()

    def close(self):
        self.run = False
        self.thread.join()
        self.thread = None
        super().close()
        self.send_queue_receive_socket.close()
        self.send_queue_receive_socket = None
        self.send_queue_send_socket.close()
        self.send_queue_send_socket = None
        self.socket.close()
        self.socket = None

    def run_server(self):
        self.server_socket = socket.socket(self.family, socket.SOCK_STREAM)
        bind_args = (self.host, self.port) if self.family == socket.AF_INET else (self.host, self.port, 0, 0)
        self.server_socket.bind(bind_args)
        self.server_socket.listen()
        self.socket, address = self.server_socket.accept()
        print(f"server accept from {address}")
        self.run_loop_in_thread()

    def run_client(self):
        self.socket = socket.socket(self.family, socket.SOCK_STREAM)
        connect_args = (self.host, self.port) if self.family == socket.AF_INET else (self.host, self.port, 0, 0)
        self.socket.connect(connect_args)
        print(f"client connect to {self.host}:{self.port}")
        self.run_loop_in_thread()


class Stream:

    should_log = False
    test_mode = False
    test_drop_fraction = 0.5

    @staticmethod
    def log(message):
        if Stream.should_log:
            print(f"Stream: {time.time()}: {message}")

    class Client:
        def connected(self, stream):
            pass
        def disconnected(self, stream):
            pass
        def received_connect(self, stream):
            pass
        def received_disconnect(self, stream):
            pass
        def received_keep_alive(self, stream):
            pass
        def received_data(self, stream, data):
            pass
        def received_ack(self, stream):
            pass
        def sent_disconnect(self, stream):
            pass
        def send_command(self, stream, data):
            return False

    version = 2

    header_type_command = 0x80000000

    command_connect_request = 0x00000000
    command_connect_response = 0x00000001
    command_disconnect = 0x00000002
    command_keep_alive = 0x00000004
    command_ack = 0x00000006
    command_nack = 0x00000008
    command_ackack = 0x0000000a

    class Tracker:

        def __init__(self, timeout):
            self.timeout = timeout
            self.last_active_time = 0.0

        def reset(self):
            self.last_active_time = 0.0

        def update(self, time):
            self.last_active_time = time

        def check(self, time):
            delta = time - self.last_active_time
            return delta <= self.timeout
        
    class State(Enum):
        disconnected = 0
        connected = 1

    class Receive:

        def __init__(self):
            self.sequence_number = 0
            self.keep_alive = Stream.Tracker(5.0)

    class Send:

        def __init__(self):
            self.sequence_number = 0
            self.keep_alive = Stream.Tracker(2.0)

    class SendTimeout(Exception):

        def __init__(self):
            super().__init__("stream send timeout")

    class LastSendData:

        def __init__(self, time, command, timeout):
            self.time = time
            self.command = command
            self.timeout = timeout

    def __init__(self, client):
        self.client = client
        self.state = Stream.State.disconnected
        self.receive = Stream.Receive()
        self.send = Stream.Send()
        self.send_lock = threading.Lock()
        self.send_ack_timeout = 5.0
        self.last_send_data = None
        self.last_send_data_timeout = 0.5

    def connected(self, configuration):
        self.disconnect()
        now = time.time()
        self.receive.keep_alive.update(now)
        self.receive.keep_alive.timeout = configuration.receive_keep_alive.float_value
        self.send.keep_alive.update(now)
        self.send.keep_alive.timeout = configuration.send_keep_alive.float_value
        self.last_send_data = None
        self.state = Stream.State.connected
        self.client.connected(self)
        self.client.received_connect(self)

    def disconnect(self):
        if self.state == Stream.State.disconnected:
            return
        if self.send_lock.locked():
            self.send_lock.release()
        self.receive.sequence_number = 0
        self.receive.keep_alive.reset()
        self.send.sequence_number = 0
        self.send.keep_alive.reset()
        self.last_send_data = None
        self.state = Stream.State.disconnected
        self.client.disconnected(self)

    def send_command(self, data):
        self.send.keep_alive.update(time.time())

        if Stream.test_mode:
            r = random.random()
            if r < Stream.test_drop_fraction:
                print("TEST: drop send")
                return

        self.client.send_command(self, data)

    def send_connect_request(self, version, connection_configuration):
        now = time.time()
        Stream.log("send connect request")
        if version <= 1:
            data = struct.pack("<LBBH",
                Stream.command_connect_request | Stream.header_type_command,
                Stream.version,
                1,  # maximum flow window size 1
                1024  # MTU
            )
        else:
            data = struct.pack("<LB",
                Stream.command_connect_request | Stream.header_type_command,
                Stream.version
            )
            data += connection_configuration.SerializeToString()
        self.send_command(data)

    def send_connect_response(self, version, connection_configuration):
        Stream.log("send connect response")
        if version <= 1:
            data = struct.pack("<LBBBH",
                    Stream.command_connect_response | Stream.header_type_command,
                    0x00,  # result accepted
                    Stream.version,
                    1,  # maximum flow window size 1
                    1024  # MTU
            )
        else:
            data = struct.pack("<LB",
                    Stream.command_connect_response | Stream.header_type_command,
                    0x00,  # result accepted
                    Stream.version,
            )
            data += connection_configuration.SerializeToString()
        self.send_command(data)

    def send_disconnect(self):
        Stream.log("send disconnect")
        data = struct.pack("<L", Stream.command_disconnect | Stream.header_type_command)
        self.send_command(data)

        self.disconnect()
        self.client.sent_disconnect(self)

    def send_keep_alive(self):
        Stream.log("send keep alive")
        data = struct.pack("<L", Stream.command_keep_alive | Stream.header_type_command)
        self.send_command(data)

    def send_ack(self, sequence_number):
        Stream.log(f"send ack: sn {sequence_number}")
        data = struct.pack("<LL", Stream.command_ack | Stream.header_type_command, sequence_number)
        self.send_command(data)

    def send_nack(self):
        Stream.log(f"send nack: sn {self.send.sequence_number}")
        data = struct.pack("<LL", Stream.command_nack | Stream.header_type_command, self.send.sequence_number)
        self.send_command(data)

    def wait_for_send_ack(self):
        if not self.send_lock.acquire(timeout=self.send_ack_timeout):
            raise Stream.SendTimeout()

    def send_data(self, data):
        now = time.time()
        self.send.keep_alive.update(now)
        Stream.log(f"send data: sn {self.send.sequence_number}, len {len(data)}")
        header = struct.pack("<L", self.send.sequence_number)
        self.last_send_data = Stream.LastSendData(now, header + data, self.last_send_data_timeout)
        self.send_command(self.last_send_data.command)

    def receive_ack(self, data):
        serial_number = struct.unpack("<L", data[0:4])[0] & ~0x80000000
        Stream.log(f"receive ack: sn {serial_number}")
        if serial_number < self.send.sequence_number:
            # duplicate ack - ignore it
            return
        if serial_number == self.send.sequence_number:
            self.last_send_data = None
            self.send.sequence_number += 1
            self.send_lock.release()
            self.client.received_ack(self)
            return
        # should not happen
        self.send_disconnect()

    def receive_nack(self, data):
        Stream.log("receive nack")
        # MFW is 1, so a nack should not happen
        self.send_disconnect()

    def receive_connect_request(self, data):
        Stream.log("receive connect request")
        version, = struct.unpack("<B", data[:1])
        if version <= 1:
            maximum_flow_window, maximum_transmission_unit = struct.unpack("<BH", data[1:3])
            configuration = stream_pb2.ConnectionConfiguration()
            configuration.maximum_flow_window.uint32_value = maximum_flow_window
            configuration.maximum_transmission_unit.uint32_value = maximum_transmission_unit
            configuration.receive_keep_alive.float_value = 5.0
            configuration.send_keep_alive.float_value = 2.0
            configuration.send_retry_timeout.float_value = 10.0
        else:
            configuration = stream_pb2.ConnectionConfiguration()
            configuration.ParseFromString(data[1:])
        self.connected(configuration)
        self.send_connect_response(version, configuration)

    def receive_connect_response(self, data):
        Stream.log("receive connect response")
        result, version = struct.unpack("<BB", data[:2])
        if result != 0:
            self.client.disconnected(self)
            return
        if version <= 1:
            maximum_flow_window, maximum_transmission_unit = struct.unpack("<BH", data[2:4])
            configuration = stream_pb2.ConnectionConfiguration()
            configuration.maximum_flow_window.uint32_value = maximum_flow_window
            configuration.maximum_transmission_unit.uint32_value = maximum_transmission_unit
            configuration.receive_keep_alive.float_value = 5.0
            configuration.send_keep_alive.float_value = 2.0
            configuration.send_retry_timeout.float_value = 10.0
        else:
            configuration = stream_pb2.ConnectionConfiguration()
            configuration.ParseFromString(data[2:])
        print(f"{configuration}")
        self.connected(configuration)

    def receive_disconnect_request(self):
        Stream.log("receive disconnect request")
        self.disconnect()
        self.client.received_disconnect(self)

    def receive_keep_alive(self):
        self.client.received_keep_alive(self)

    def receive_data(self, sequence_number, data):
        Stream.log(f"receive data: sn {sequence_number}, len {len(data)})")
        if sequence_number < self.receive.sequence_number:
            # retransmitted data - ignore
            self.send_ack(sequence_number)
            return
        if sequence_number == self.receive.sequence_number:
            # next data in the sequence - process it
            self.send_ack(self.receive.sequence_number)
            self.client.received_data(self, data);
            self.receive.sequence_number += 1
            return
        # data gap detected
        self.send_nack()

    def receive_message(self, data):
        if Stream.test_mode:
            r = random.random()
            if r < Stream.test_drop_fraction:
                print("TEST: drop receive")
                return

        self.receive.keep_alive.update(time.time())

        if len(data) < 4:
            return
        header = struct.unpack("<L", data[0:4])[0]
        if (header & Stream.header_type_command) != 0:
            command = header & ~Stream.header_type_command
            match command:
                case Stream.command_connect_request:
                    self.receive_connect_request(data[4:])
                case Stream.command_connect_response:
                    self.receive_connect_response(data[4:])
                case Stream.command_disconnect:
                    self.receive_disconnect_request()
                case Stream.command_keep_alive:
                    self.receive_keep_alive()
                case Stream.command_ack:
                    self.receive_ack(data[4:])
                case Stream.command_nack:
                    self.receive_nack(data[4:])
                case Stream.command_ackack:
                    print("unimplemented command")
                case _:
                    print("unknown command")
        else:
            if self.state == Stream.State.connected:
                self.receive_data(header, data[4:])

    def check(self):
        if self.state == Stream.State.disconnected:
            return

        now = time.time()
        if self.last_send_data is not None:
            deadline = self.last_send_data.time + self.last_send_data.timeout
            if now > deadline:
                self.last_send_data.timeout *= 2.0
                Stream.log("resend last data")
                self.send_command(self.last_send_data.command)

        if not self.send.keep_alive.check(now):
            self.send_keep_alive()
        if not self.receive.keep_alive.check(now):
            Stream.log("keep alive: disconnect")
            self.send_disconnect()


class UdpChannel(Transport.StreamChannel):

    should_log = False

    @staticmethod
    def log(message):
        if Stream.should_log:
            print(f"UDP Channel: {time.time()}: {message}")

    class Client:
        def connected(self, udp_channel):
            pass
        def disconnected(self, udp_channel):
            pass

    class Options:
        def __init__(self):
            self.connect_when_run = True
            self.client = None
            self.send_keep_alive_interval = 2.0
            self.receive_keep_alive_interval = 5.0
            self.send_ack_timeout = 5.0
            self.send_retry_timeout = 0.5
            
    class ConnectTimeout(Exception):

        def __init__(self):
            super().__init__("UDP Channel Connect Timeout")

    def __init__(self, transport, host, port, family=socket.AF_INET, options=None):
        super().__init__(transport)
        self.host = host
        self.port = port
        self.family = family
        self.server_socket = None
        self.socket = None
        self.run = False
        self.thread = None
        self.send_queue = queue.Queue()
        self.send_queue_receive_socket, self.send_queue_send_socket = socket.socketpair()
        self.stream = Stream(self)
        self.connect_queue = queue.Queue()
        self.connect_when_run = True
        self.client = None
        if options is not None:
            self.connect_when_run = options.connect_when_run
            self.client = options.client
            self.stream.send.keep_alive.timeout = options.send_keep_alive_interval
            self.stream.receive.keep_alive.timeout = options.receive_keep_alive_interval
            self.stream.send_ack_timeout = options.send_ack_timeout
            self.stream.last_send_data_timeout = options.send_retry_timeout

    def is_connected(self):
        return self.stream.state == Stream.State.connected
    
    def connected(self, stream):
        if self.client is not None:
            self.client.connected(self)
    
    def disconnected(self, stream):
        if self.client is not None:
            self.client.disconnected(self)
    
    def received_connect(self, stream):
        UdpChannel.log("received connect")
        self.connect_queue.put(0)

    def received_disconnect(self, stream):
        UdpChannel.log("received disconnect")

    def received_keep_alive(self, stream):
        UdpChannel.log("received keep alive")

    def received_data(self, stream, data):
        UdpChannel.log(f"received data {data}")
        self.read(data)

    def received_ack(self, stream):
        pass

    def sent_disconnect(self, stream):
        UdpChannel.log("sent disconnect")

    def send_command(self, stream, data):
        address = (self.host, self.port) if self.family == socket.AF_INET else (self.host, self.port, 0, 0)
        length = self.socket.sendto(data, address)
        if length != len(data):
            UdpChannel.log("send failed")

    def write(self, data):
        UdpChannel.log(f"write {len(data)}")

        # need to block if stream is waiting for an ack
        self.stream.wait_for_send_ack()

        self.send_queue.put(data)
        self.send_queue_send_socket.send(b"\x00")

    def run_loop(self):
        while self.run:
            ready_sockets, _, _ = select.select([self.socket, self.send_queue_receive_socket], [], [], 0.1)
            # UdpChannel.log(f"ready sockets {repr(ready_sockets)}")
            for ready_socket in ready_sockets:
                if ready_socket is self.socket:
                    data, address = self.socket.recvfrom(4 + 1024)  # header + data
                    UdpChannel.log(f"socket recvfrom {address} {len(data)}")
                    self.stream.receive_message(data)
                if ready_socket is self.send_queue_receive_socket:
                    self.send_queue_receive_socket.recv(1)
                    data = self.send_queue.get()
                    UdpChannel.log(f"stream send {len(data)}")
                    self.stream.send_data(data)
                    UdpChannel.log(f"stream send done")
            self.stream.check()

    def connect(self, timeout=5.0, connection_configuration=None):
        version = 1 if connection_configuration is None else Stream.version
        self.stream.send_connect_request(version=version, connection_configuration=connection_configuration)
        try:
            self.connect_queue.get(timeout=timeout)
        except queue.Empty:
            raise UdpChannel.ConnectTimeout()

    def run_loop_in_thread(self):
        self.run = True
        self.thread = threading.Thread(target=self.run_loop, args=())
        self.thread.start()
        if self.connect_when_run:
            self.connect()

    def close(self):
        self.run = False
        self.thread.join()
        self.thread = None
        super().close()
        self.send_queue_receive_socket.close()
        self.send_queue_receive_socket = None
        self.send_queue_send_socket.close()
        self.send_queue_send_socket = None
        self.socket.close()
        self.socket = None

    def run_client(self):
        self.socket = socket.socket(self.family, socket.SOCK_DGRAM)
        connect_args = (self.host, self.port) if self.family == socket.AF_INET else (self.host, self.port, 0, 0)
        self.socket.connect(connect_args)
        print(f"sent connect request to {self.host}:{self.port}")
        self.run_loop_in_thread()


class RPC:

    should_log = False

    @staticmethod
    def log(message):
        if RPC.should_log:
            print(f"RPC: {time.time()}: {message}")

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
            raise RPC.TimeoutError(f"unary call timeout {timeout}")
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
