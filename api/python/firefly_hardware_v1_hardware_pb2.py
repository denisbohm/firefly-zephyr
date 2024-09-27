import hardware_pb2

from rpc import RPC


class Hardware:

    hardware_get_device_identifier = RPC.Method(8, 1, 1, hardware_pb2.GetDeviceIdentifierRequest, hardware_pb2.GetDeviceIdentifierResponse, False, False, 0)

    @staticmethod
    def get_device_identifier(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_get_device_identifier, request, channel, rpc, timeout)

    hardware_configure_port = RPC.Method(8, 1, 2, hardware_pb2.ConfigurePortRequest, hardware_pb2.ConfigurePortResponse, False, False, 0)

    @staticmethod
    def configure_port(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_configure_port, request, channel, rpc, timeout)

    hardware_get_port = RPC.Method(8, 1, 3, hardware_pb2.GetPortRequest, hardware_pb2.GetPortResponse, False, False, 0)

    @staticmethod
    def get_port(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_get_port, request, channel, rpc, timeout)

    hardware_set_port = RPC.Method(8, 1, 4, hardware_pb2.SetPortRequest, hardware_pb2.SetPortResponse, False, False, 0)

    @staticmethod
    def set_port(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_set_port, request, channel, rpc, timeout)

    hardware_spi_transceive = RPC.Method(8, 1, 5, hardware_pb2.SpiTransceiveRequest, hardware_pb2.SpiTransceiveResponse, False, False, 0)

    @staticmethod
    def spi_transceive(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_spi_transceive, request, channel, rpc, timeout)

    hardware_i2c_transfer = RPC.Method(8, 1, 6, hardware_pb2.I2cTransferRequest, hardware_pb2.I2cTransferResponse, False, False, 0)

    @staticmethod
    def i2c_transfer(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(Hardware.hardware_i2c_transfer, request, channel, rpc, timeout)
