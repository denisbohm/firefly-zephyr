import struct


class SystemVersion:

    def __init__(self, major, minor, patch):
        self.major = major
        self.minor = minor
        self.patch = patch


class System:

    operation_get_version = 0

    @staticmethod
    def encode_io():
        data = bytearray()
        data += struct.pack("<B", System.operation_get_version)
        return data

    @staticmethod
    def decode_io(data):
        major, minor, patch = struct.unpack("<III", data)
        return SystemVersion(major, minor, patch)
