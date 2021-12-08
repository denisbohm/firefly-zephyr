from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway
from firefly.transport.spim import SPIM
from firefly.transport.spim import SPIMTransaction

import struct
import sys
import time


class Main:

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
        transactions = [SPIMTransaction(tx_data=bytes([(register << 1) | 0x01, (value >> 8) & 0xff, value & 0xff]))]
        self.spim_io(device, transactions)

    def check_photometric_front_end(self, device):
        value = self.pfe_read(device, 0x08)
        rev_num = value >> 8
        dev_id = value & 0xff
        print(f"PFE{device} rev_num 0x{rev_num:02x} (0x0a expected)")
        print(f"PFE{device} dev_id 0x{dev_id:02x} (0x16 expected)")

    def run(self):
        device = Main.spim_device_pfe0
        self.check_photometric_front_end(device)

        self.pfe_write(device, 0x0f, 0x0001)  # Reset Device
        self.pfe_write(device, 0x10, 0x0001)  # Enter Program Mode
        self.pfe_write(device, 0x4B, 0x0080)  # Enable 32 kHz clock

        # Enable slot A & B w/ FIFO
        #  self.pfe_write(device, 0x11, 0x0021)
        self.pfe_write(device, 0x11, 0x0065)
        # AFE Window Settings
        self.pfe_write(device, 0x30, 0x0219)
        self.pfe_write(device, 0x35, 0x0219)
        self.pfe_write(device, 0x39, 0x1A08)
        self.pfe_write(device, 0x3B, 0x1A08)

        ledxi = 1
        # Use PD5 Channel 1 in Time Slot A & B (LEDXi)
        self.pfe_write(device, 0x14, 0x0440 | (ledxi << 2) | ledxi)
        # Enter Normal Sampling Mode
        self.pfe_write(device, 0x10, 0x0002)
        # wait for some samples
        time.sleep(0.001)
        # Hold Data for Slot A & B
        self.pfe_write(device, 0x5F, 0x0006)

        # get fifo data


if __name__ == '__main__':
    main = Main()
    main.run()
