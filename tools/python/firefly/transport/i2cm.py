import struct


class I2CMTransaction:

    direction_rx = 0
    direction_tx = 1

    def __init__(self, direction, count, data):
        self.direction = direction
        self.count = count
        self.data = data

    @staticmethod
    def rx(count):
        return I2CMTransaction(I2CMTransaction.direction_rx, count, None)

    @staticmethod
    def tx(data):
        return I2CMTransaction(I2CMTransaction.direction_tx, len(data), data)


class I2CM:

    result_success = 0
    result_nack = 1

    operation_io = 0

    @staticmethod
    def encode_io(device, transactions):
        data = bytearray()
        data += struct.encode("<BBB", I2CM.operation_io, device, len(transactions))
        for transaction in transactions:
            data += struct.encode("<BB", transaction.direction, transaction.count)
            if transaction.direction == I2CMTransaction.direction_tx:
                data += transaction.data
        return data

    @staticmethod
    def decode_io(device, transactions, data):
        operation, result, io_device, transaction_count = struct.decode("<BBBB", data[0:3])
        if operation != I2CM.operation_io:
            raise Exception("unexpected operation")
        if result != I2CM.result_success:
            raise Exception("unexpected result {result}")
        if io_device != device:
            raise Exception("unexpected device")
        if transaction_count != len(transactions):
            raise Exception("unexpected transactions")
        index = 3
        for transaction in transactions:
            direction, count = struct.decode("<BB", data[index:index + 2])
            index += 2
            if direction != transaction.direction:
                raise Exception("unexpected direction")
            if count != transaction.count:
                raise Exception("unexpected count")
            if direction == I2CMTransaction.direction_rx:
                transaction.data = data[index:index + count]
                index += count
