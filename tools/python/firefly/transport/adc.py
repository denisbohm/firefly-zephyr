import struct


class ADC:

    operation_convert = 0

    @staticmethod
    def encode_convert(channel, max_voltage):
        return struct.pack("<BIf", ADC.operation_convert, channel, max_voltage)

    @staticmethod
    def decode_convert(data):
        operation, channel, voltage = struct.unpack("<BIf", data)
        if operation != ADC.operation_convert:
            raise Exception("unexpected operation")
        return voltage
