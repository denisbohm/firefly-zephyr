from cobs import cobs
from firefly.transport.envelope import Envelope
import serial
import serial.tools.list_ports
import struct


class GatewayException(Exception):
    pass


class Gateway:

    @staticmethod
    def find_serial_port(vid=0x2FE3, pid=0x0100):
        for info in serial.tools.list_ports.comports():
            if info.vid == vid and info.pid == pid:
                return info.device
        return None

    def __init__(self, port):
        self.port = port
        self.serial_port = serial.Serial(
            port=port,
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=0.5
        )
        self.waiting_message = bytearray()
        self.trace = False

    def crc16(self, data):
        crc = 0
        for i in range(len(data)):
            byte = data[i]
            crc ^= byte << 8
            for _ in range(8):
                temp = crc << 1
                if (crc & 0x8000) != 0:
                    temp ^= 0x1021
                crc = temp
        return crc & 0xffff

    def add_envelope(self, message, envelope):
        envelope.length = len(message)
        data = message + struct.pack(
            "<BBBBBBH",
            envelope.reserved0,
            envelope.type,
            envelope.subsystem,
            envelope.system,
            envelope.source,
            envelope.target,
            envelope.length
        )
        envelope.crc16 = self.crc16(data)
        encoded = data + struct.pack("<H", envelope.crc16)
        return encoded

    def get_envelope(self, message):
        if len(message) < 10:
            raise GatewayException("invalid envelope")
        data = message[len(message) - 10:]
        reserved0, type, subsystem, system, source, target, length, crc16 = struct.unpack("<BBBBBBHH", data)
        if length != (len(message) - 10):
            raise GatewayException("invalid envelope")
        actual_crc16 = self.crc16(message[0:len(message) - 2])
        if crc16 != actual_crc16:
            raise GatewayException("invalid envelope")
        return message[0:length], Envelope(crc16, length, target, source, system, subsystem, type, reserved0)

    def tx(self, data, envelope):
        enveloped = self.add_envelope(data, envelope)
        if self.trace:
            print(f"tx enveloped {enveloped.hex()}")
        encoded = cobs.encode(enveloped)
        if self.trace:
            print(f"tx encoded {encoded.hex()}")
        #  self.serial_port.write(b'\x00')
        raw = b'\x00' + encoded + b'\x00'
        self.serial_port.write(raw)
        self.serial_port.flush()

    def rx(self):
        message = self.waiting_message
        self.waiting_message = bytearray()
        while True:
            data = self.serial_port.read(1)
            if len(data) != 1:
                if self.trace:
                    print(f"rx (timed out) {data.hex()}")
                raise GatewayException("read timeout")
            if data[0] == 0:
                if len(message) > 0:
                    decoded = cobs.decode(message)
                    if self.trace:
                        print(f"rx {decoded.hex() }")
                    deenveloped, envelope = self.get_envelope(decoded)
                    return deenveloped, envelope
            else:
                message.extend(data)

    def rx_waiting(self):
        while True:
            count = self.serial_port.in_waiting
            if count == 0:
                return None
            data = self.serial_port.read(1)
            if len(data) != 1:
                if self.trace:
                    print(f"rx (timed out) {data.hex()}")
                raise GatewayException("read timeout")
            if data[0] == 0:
                if len(self.waiting_message) > 0:
                    message = self.waiting_message
                    self.waiting_message = bytearray()
                    decoded = cobs.decode(message)
                    if self.trace:
                        print(f"rx {decoded.hex() }")
                    deenveloped, envelope = self.get_envelope(decoded)
                    return deenveloped, envelope
            else:
                self.waiting_message.extend(data)

    def rpc(self, request, request_envelope):
        self.tx(request, request_envelope)
        return self.rx()

