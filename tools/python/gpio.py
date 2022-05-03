from firefly.transport.envelope import Envelope
from firefly.transport.gateway import GatewaySerial
from firefly.transport.gpio import GPIO
from firefly.transport.gpio import GPIOTransaction
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

    def gpio_io(self, transactions):
        request = GPIO.encode_io(transactions)
        request_envelope = Envelope(
            target=main.target_board,
            source=main.target_usb,
            system=Envelope.system_firefly,
            subsystem=Envelope.subsystem_gpio,
            type=Envelope.type_request
        )
        self.gateway.tx(request, request_envelope)
        response, response_envelope = self.gateway.rx()
        GPIO.decode_io(transactions, response)

    def gpio_get(self, port, pin):
        transactions = [
            GPIOTransaction.configure(port, pin, GPIO.direction_input),
            GPIOTransaction.get(port, pin),
        ]
        self.gpio_io(transactions)
        return transactions[1].value

    def gpio_set(self, port, pin, value):
        configuration = GPIO.direction_output | GPIO.drive_both | (GPIO.level_high if value else GPIO.level_low)
        transactions = [
            GPIOTransaction.configure(port, pin, configuration)
        ]
        self.gpio_io(transactions)

    def blink(self, port, pin):
        for _ in range(0, 5):
            self.gpio_set(port, pin, value=True)
            time.sleep(0.1)
            self.gpio_set(port, pin, value=False)
            time.sleep(0.1)

    def run(self):
        print("blink red")
        self.blink(port=0, pin=30)
        print("blink green")
        self.blink(port=1, pin=11)
        print("blink blue")
        self.blink(port=0, pin=31)

        for _ in range(0, 10):
            time.sleep(0.25)
            value = self.gpio_get(port=1, pin=10)
            print(f"right button {value}")


if __name__ == '__main__':
    main = Main()
    main.run()
