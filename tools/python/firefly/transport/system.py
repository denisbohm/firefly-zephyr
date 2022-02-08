import struct


class SystemVersion:

    def __init__(self, major, minor, patch):
        self.major = major
        self.minor = minor
        self.patch = patch


class SystemIdentity:

    def __init__(self, version, identifier):
        self.version = version
        self.identifier = identifier


class System:

    operation_get_identity = 0
    operation_get_assert = 1

    # for backwards compatibility only -denis
    class GetVersion:

        @staticmethod
        def encode():
            data = bytearray()
            data += struct.pack("<B", System.operation_get_identity)
            return data

        @staticmethod
        def decode(data):
            operation, major, minor, patch = struct.unpack("<BIII", data[0:13])
            if operation != System.operation_get_identity:
                raise Exception("unexpected operation")
            return SystemVersion(major, minor, patch)

    class GetIdentity:

        @staticmethod
        def encode():
            data = bytearray()
            data += struct.pack("<B", System.operation_get_identity)
            return data

        @staticmethod
        def decode(data):
            operation, major, minor, patch, identifier_length = struct.unpack("<BIIIB", data[0:14])
            if operation != System.operation_get_identity:
                raise Exception("unexpected operation")
            identifier = data[14:14 + identifier_length].decode("utf-8")
            return SystemIdentity(SystemVersion(major, minor, patch), identifier)

    class Assert:

        class Failure:

            def __init__(self, file, line, message):
                self.file = file
                self.line = line
                self.message = message

        def __init__(self, count, failures):
            self.count = count
            self.failures = failures

    class GetAssert:

        @staticmethod
        def encode():
            data = bytearray()
            data += struct.pack("<B", System.operation_get_assert)
            return data

        @staticmethod
        def decode(data):
            operation, count = struct.unpack("<BI", data[0:5])
            if operation != System.operation_get_assert:
                raise Exception("unexpected operation")
            failures = []
            index = 5
            while index < len(data):
                length = data[index]
                index += 1
                file = data[index: index + length].decode('utf-8')
                index += length
                line = struct.unpack("<I", data[index: index + 4])[0]
                index += 4
                length = data[index]
                index += 1
                message = data[index: index + length].decode('utf-8')
                index += length
                failures.append(System.Assert.Failure(file, line, message))
            return System.Assert(count, failures)

    @staticmethod
    def get_assert(gateway, envelope):
        response, response_envelope = gateway.rpc(System.GetAssert.encode(), envelope)
        return System.GetAssert.decode(response)
