import struct


class GPIOTransaction:

    def __init__(self, opcode, port, pin, configuration=0, value=False):
        self.opcode = opcode
        self.port = port
        self.pin = pin
        self.configuration = configuration
        self.value = value

    @staticmethod
    def configure(port, pin, configuration):
        return GPIOTransaction(GPIO.opcode_configure, port, pin, configuration=configuration)

    @staticmethod
    def set(port, pin, value):
        return GPIOTransaction(GPIO.opcode_set, port, pin, value=value)

    @staticmethod
    def get(port, pin):
        return GPIOTransaction(GPIO.opcode_get, port, pin)


class GPIO:

    operation_io = 0

    opcode_configure = 0
    opcode_set = 1
    opcode_get = 2

    direction_none   = 0b00
    direction_input  = 0b01
    direction_output = 0b10

    pull_none = 0b00 << 2
    pull_down = 0b01 << 2
    pull_up   = 0b10 << 2

    level_low  = 0b0 << 4
    level_high = 0b1 << 4

    drive_none = 0b00 << 5
    drive_low  = 0b01 << 5
    drive_high = 0b10 << 5
    drive_both = 0b11 << 5

    @staticmethod
    def encode_io(transactions):
        data = bytearray()
        data += struct.pack("<B", GPIO.operation_io)
        for transaction in transactions:
            data += struct.pack("<BBB", transaction.opcode, transaction.port, transaction.pin)
            if transaction.opcode == GPIO.opcode_configure:
                data += struct.pack("<B", transaction.configuration)
            elif transaction.opcode == GPIO.opcode_set:
                data += struct.pack("<B", 1 if transaction.value else 0)
        return data

    @staticmethod
    def decode_io(transactions, data):
        operation = data[0]
        if operation != GPIO.operation_io:
            raise Exception("unexpected operation")
        index = 1
        for transaction in transactions:
            if transaction.opcode == GPIO.opcode_get:
                opcode = data[index]
                index += 1
                if opcode != GPIO.opcode_get:
                    raise Exception("unexpected opcode")
                port = data[index]
                index += 1
                if port != transaction.port:
                    raise Exception("unexpected port")
                pin = data[index]
                index += 1
                if pin != transaction.pin:
                    raise Exception("unexpected pin")
                transaction.value = data[index] != 0
                index += 1
