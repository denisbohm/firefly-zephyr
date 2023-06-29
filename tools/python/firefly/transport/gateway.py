from cobs import cobs
from firefly.transport.envelope import Envelope
from pyftdi.i2c import I2cController
from pyftdi.i2c import I2cNackError
import serial
import serial.tools.list_ports
import struct
import time

class GatewayException(Exception):
    pass


class Gateway:

    def __init__(self):
        self.waiting_message = bytearray()
        self.trace = False

    def enter_boot_loader(self):
        raise GatewayException("unimplemented")

    def set_timeout(self, timeout):
        raise GatewayException("unimplemented")

    def get_timeout(self):
        raise GatewayException("unimplemented")

    def write(self, data):
        raise GatewayException("unimplemented")

    def read(self, n):
        raise GatewayException("unimplemented")

    def in_waiting(self):
        raise GatewayException("unimplemented")

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

    def to_raw(self, data, envelope):
        enveloped = self.add_envelope(data, envelope)
        if self.trace:
            print(f"tx enveloped {enveloped.hex()}")
        encoded = cobs.encode(enveloped)
        if self.trace:
            print(f"tx encoded {encoded.hex()}")
        #  self.serial_port.write(b'\x00')
        raw = b'\x00' + encoded + b'\x00'
        return raw

    def tx(self, data, envelope):
        raw = self.to_raw(data, envelope)
        self.write(raw)

    def rx(self):
        message = self.waiting_message
        self.waiting_message = bytearray()
        while True:
            data = self.read(1)
            if len(data) == 0:
                if self.trace:
                    print(f"rx (timed out) {data.hex()}")
                raise GatewayException("read timeout")
            if data[0] == 0:
                if len(message) > 0:
                    if self.trace:
                        print(f"rx encoded {message.hex()}")
                    decoded = cobs.decode(message)
                    if self.trace:
                        print(f"rx {decoded.hex()}")
                    deenveloped, envelope = self.get_envelope(decoded)
                    return deenveloped, envelope
            else:
                message.extend(data)

    def rx_waiting(self):
        while True:
            count = self.in_waiting()
            if count == 0:
                return None
            data = self.read(1)
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
                        print(f"rx {decoded.hex()}")
                    deenveloped, envelope = self.get_envelope(decoded)
                    return deenveloped, envelope
            else:
                self.waiting_message.extend(data)

    def rpc(self, request, request_envelope):
        self.tx(request, request_envelope)
        return self.rx()


class GatewayI2C(Gateway):

    def __init__(self, address, url='ftdi:///1'):
        super().__init__()
        i2c = I2cController()
        i2c.set_retry_count(1)
        i2c.configure(url, frequency=100000, clockstretching=True)
        self.i2c = i2c
        self.port = i2c.get_port(address)
        self.rx_data = bytearray()
        self.timeout = 5.0

    def set_timeout(self, timeout):
        self.timeout = timeout

    def get_timeout(self):
        return self.timeout

    def enter_boot_loader(self):
        D3_PIN = 3
        port = self.i2c.get_gpio()
        port.set_direction(pins=(1<<D3_PIN), direction=(1<<D3_PIN)) # config AD3 as output
        pins = port.read()
        pins &= ~(1 << D3_PIN) # config AD3 as LOW to trigger reset
        port.write(pins) 
        time.sleep(0.25)
        pins = port.read()
        pins |= 1 << D3_PIN # config D3 as HIGH to enable PMM
        port.write(pins) 
        time.sleep(0.25)

    def write(self, data):
        self.port.write(out=data)

    def i2c_read(self):
        limit = 32
        try:
            response = self.port.read(readlen=limit)
            length = response[0]
        except I2cNackError:
            return bytes()
        if length >= limit:
            length = limit - 1
        return response[1 : 1 + length]

    def read(self, n):
        start = time.time()
        while True:
            if len(self.rx_data) >= n:
                data = self.rx_data[0 : n]
                del self.rx_data[0 : n]
                return data
            response = self.i2c_read()
            if len(response) == 0:
                duration = time.time() - start
                if duration > 1000:
                    raise ControllerException("timeout")
                continue
            start = time.time()
            self.rx_data.extend(response)

    def in_waiting(self):
        length = len(self.rx_data)
        if length > 0:
            return length
        response = self.i2c_read()
        self.rx_data.extend(response)
        return len(self.rx_data)


class GatewaySerial(Gateway):

    @staticmethod
    def find_serial_port(vid=0x2FE3, pid=0x0100):
        for info in serial.tools.list_ports.comports():
            if info.vid == vid and info.pid == pid:
                return info.device
        return None

    def __init__(self, port):
        super().__init__()
        self.port = port
        self.serial_port = serial.Serial(
            port=port,
            baudrate=115200,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=5.0
        )

    def set_timeout(self, timeout):
        self.serial_port.timeout = timeout

    def get_timeout(self):
        return self.serial_port.timeout

    def enter_boot_loader(self):
        port = self.serial_port
        port.break_condition = True
        port.rts = True
        time.sleep(0.25)
        port.rts = False
        time.sleep(0.25)
        port.break_condition = False

    def write(self, data):
        self.serial_port.write(data)
        self.serial_port.flush()

    def read(self, n):
        return self.serial_port.read(n)

    def in_waiting(self):
        return self.serial_port.in_waiting


