from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway
from firefly.transport.system import System

import struct
import sys
import time


class Main:

    target_main = 0
    target_sensor_0 = 1
    target_sensor_1 = 2
    target_sensor_2 = 3
    target_usb = 4

    def __init__(self):
        # FTDI USB TTL-232
        self.port = Gateway.find_serial_port(vid=0x0403, pid=0x6001)
        # Zephyr USB CDC
        #  self.port = Gateway.find_serial_port(vid=0x2FE3, pid=0x0100)
        if not self.port:
            print(f"cannot find USB serial port")
            sys.exit(1)
        print(f"using USB serial port {self.port}")
        self.gateway = Gateway(self.port)

    def run(self):
        operation = System.operation_get_version
        major = 1
        minor = 2
        patch = 3
        response = struct.pack("<BIII", operation, major, minor, patch)
        response_envelope = Envelope(
            target=main.target_usb,
            source=main.target_sensor_0,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_system,
            type=Envelope.type_request
        )
        self.gateway.tx(response, response_envelope)
        time.sleep(1)


if __name__ == '__main__':
    main = Main()
    main.run()
