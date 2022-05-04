from firefly.transport.envelope import Envelope
from firefly.transport.gateway import GatewaySerial
from firefly.transport.adc import ADC

import sys
import time


class Main:

    target_board = 0
    target_usb = 4

    def __init__(self):
        self.port = GatewaySerial.find_serial_port()
        if not self.port:
            print(f"cannot find USB serial port")
            sys.exit(1)
        print(f"using USB serial port {self.port}")
        self.gateway = GatewaySerial(self.port)

    def adc_convert(self, channel, max_voltage):
        request = ADC.encode_convert(channel, max_voltage)
        request_envelope = Envelope(
            target=main.target_board,
            source=main.target_usb,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_adc,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        return ADC.decode_convert(response)

    def run(self):
        vdd = self.adc_convert(9, 3.6)
        print(f"VDD {vdd:0.2f}")


if __name__ == '__main__':
    main = Main()
    main.run()
