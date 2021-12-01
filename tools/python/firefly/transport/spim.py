import struct


class SPIMTransaction:

    def __init__(self, tx_data=None, rx_count=0):
        self.tx_count = len(tx_data)
        self.tx_data = tx_data
        self.rx_count = rx_count
        self.rx_data = None

    @staticmethod
    def rx(count):
        return SPIMTransaction(rx_count=count)

    @staticmethod
    def tx(data):
        return SPIMTransaction(tx_data=data)


class SPIM:

    operation_io = 0

    @staticmethod
    def encode_io(device, transactions):
        data = bytearray()
        data += struct.pack("<BBB", SPIM.operation_io, device, len(transactions))
        for transaction in transactions:
            data += struct.pack("<B", transaction.tx_count)
            data += transaction.tx_data
            data += struct.pack("<B", transaction.rx_count)
        return data

    @staticmethod
    def decode_io(device, transactions, data):
        operation, io_device, transaction_count = struct.unpack("<BBB", data[0:3])
        if operation != SPIM.operation_io:
            raise Exception("unexpected operation")
        if io_device != device:
            raise Exception("unexpected device")
        if transaction_count != len(transactions):
            raise Exception("unexpected transactions")
        index = 3
        for transaction in transactions:
            tx_count, rx_count = struct.unpack("<BB", data[index:index + 2])
            index += 2
            transaction.rx_data = data[index:index + rx_count]
            index += rx_count
