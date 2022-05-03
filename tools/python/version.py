from firefly.transport.envelope import Envelope
from firefly.transport.gateway import GatewaySerial
from firefly.transport.system import System

import sys


class Main:

    target_main = 0
    target_usb = 1
    target_ble = 2

    def __init__(self):
        # Zephyr USB CDC
        self.port = GatewaySerial.find_serial_port(vid=0x2FE3, pid=0x0100)
        if not self.port:
            print(f"cannot find USB serial port")
            sys.exit(1)
        print(f"using USB serial port {self.port}")
        self.gateway = GatewaySerial(self.port)

    def system_get_identity(self):
        request = System.GetIdentity.encode()
        request_envelope = Envelope(
            target=main.target_main,
            source=main.target_usb,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_system,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        return System.GetIdentity.decode(response)

    def run(self):
        identity = self.system_get_identity()
        version = identity.version
        print(f"{identity.identifier} {version.major}.{version.minor}.{version.patch}")


if __name__ == '__main__':
    main = Main()
    main.run()
