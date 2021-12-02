from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway
from firefly.transport.spim import SPIM
from firefly.transport.spim import SPIMTransaction
import sys


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
        transactions = [SPIMTransaction(tx_data=[(register << 1) | 0x01, (value >> 8) & 0xff, value & 0xff])]
        self.spim_io(device, transactions)

    def check_photometric_front_end(self, device):
        value = self.pfe_read(device, 0x08)
        rev_num = value & 0xff
        dev_id = value >> 8
        print(f"PFE{device} rev_num {rev_num} 0x0a")
        print(f"PFE{device} dev_id {dev_id} 0x16")

    def run(self):
        self.check_photometric_front_end(Main.spim_device_pfe0)


if __name__ == '__main__':
    main = Main()
    main.run()
