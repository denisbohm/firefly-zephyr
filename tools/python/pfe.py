from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway
from firefly.transport.spim import SPIM
from firefly.transport.spim import SPIMTransaction

import struct
import sys

class PFE:

    operation_get_fifo_data = 0

    @staticmethod
    def encode_io(sample_size=2):
        data = bytearray()
        data += struct.pack("<BB", PFE.operation_get_fifo_data, sample_size)
        return data

    @staticmethod
    def decode_io(data):
        operation, sample_size = struct.unpack("<BB", data[0:2])
        if operation != PFE.operation_get_fifo_data:
            raise Exception("unexpected operation")
        fifos = []
        index = 2
        for _ in range(2):
            sample_count = struct.unpack("<B", data[index:index + 1])[0]
            index += 1
            samples = []
            for _ in range(sample_count):
                sample = struct.unpack(">H", data[index:index + 2])[0]
                index += 2
                samples.append(sample)
            fifos.append(samples)
        return fifos

class Main:

    system_loop = 1

    subsystem_pfe = 0

    spim_device_pfe0 = 0
    spim_device_pfe1 = 1

    target_sensor_0 = 1
    target_usb = 4

    def __init__(self):
        self.port = Gateway.find_serial_port(vid=0x0403, pid=0x6001)
        if not self.port:
            print(f"cannot find USB serial port")
            sys.exit(1)
        print(f"using USB serial port {self.port}")
        self.gateway = Gateway(self.port)

    def spim_io(self, device, transactions):
        request = SPIM.encode_io(device, transactions)
        request_envelope = Envelope(
            target=main.target_sensor_0,
            source=main.target_usb,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_spim,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        SPIM.decode_io(device, transactions, response)

    def pfe_read(self, device, register):
        transactions = [SPIMTransaction(tx_data=bytes([register << 1]), rx_count=3)]
        self.spim_io(device, transactions)
        rx = transactions[0].rx_data
        value = (rx[1] << 8) | rx[2]
        return value

    def pfe_write(self, device, register, value):
        transactions = [SPIMTransaction(tx_data=[(register << 1) | 0x01, (value >> 8) & 0xff, value & 0xff])]
        self.spim_io(device, transactions)

    def check_photometric_front_end(self, device):
        value = self.pfe_read(device, 0x08)
        rev_num = value >> 8
        dev_id = value & 0xff
        print(f"PFE{device} rev_num 0x{rev_num:02x} (0x0a expected)")
        print(f"PFE{device} dev_id 0x{dev_id:02x} (0x16 expected)")

    def pfe_get_fifo_data(self):
        request = PFE.encode_io()
        request_envelope = Envelope(
            target=main.target_sensor_0,
            source=main.target_usb,
            system=Main.system_loop,
            subsystem=Main.subsystem_pfe,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        return PFE.decode_io(response)

    def run(self):
        self.check_photometric_front_end(Main.spim_device_pfe0)
        fifos = self.pfe_get_fifo_data()
        for fifo in fifos:
            print(f"fifo sample count {len(fifo)}")


if __name__ == '__main__':
    main = Main()
    main.run()
