from firefly.transport.envelope import Envelope
from firefly.transport.gateway import GatewaySerial
from firefly.transport.i2cm import I2CM
from firefly.transport.i2cm import I2CMTransaction
import sys


class Main:

    i2cm_device_imu = 1

    target_board = 0
    target_usb = 4

    def __init__(self):
        self.port = GatewaySerial.find_serial_port()
        if not self.port:
            print(f"cannot find USB serial port")
            sys.exit(1)
        print(f"using USB serial port {self.port}")
        self.gateway = GatewaySerial(self.port)

    def i2cm_io(self, device, transactions):
        request = I2CM.encode_io(device, transactions)
        request_envelope = Envelope(
            target=main.target_board,
            source=main.target_usb,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_i2cm,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        I2CM.decode_io(device, transactions, response)

    def i2cm_read(self, device, address):
        transactions = [I2CMTransaction.tx(bytes([address])), I2CMTransaction.rx(1)]
        self.i2cm_io(device, transactions)
        value = transactions[1].data[0]
        return value

    def i2cm_write(self, device, address, value):
        transactions = [I2CMTransaction.tx(bytes([address, value]))]
        self.i2cm_io(device, transactions)
        value = transactions[1].data[0]
        return value

    def run(self):
        whoami = self.i2cm_read(Main.i2cm_device_imu, 0x0f)
        print(f"imu whoami {whoami:02x}")


if __name__ == '__main__':
    main = Main()
    main.run()
