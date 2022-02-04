#!/usr/bin/env python3

from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway

import struct
import time
import argparse


class Version:

    def __init__(self, major, minor, patch):
        self.major = major
        self.minor = minor
        self.patch = patch


class Identity:

    def __init__(self, identifier, version):
        self.identifier = identifier
        self.version = version


class HostException(Exception):
    pass


class Host:

    operation_get_identity = 0
    operation_get_executable_metadata = 1
    operation_get_update_metadata = 2
    operation_update = 3
    operation_execute = 4
    operation_get_update_storage = 5
    operation_update_read = 6
    operation_status_progress = 7

    class GetIdentity:

        @staticmethod
        def encode():
            return struct.pack("<H", Host.operation_get_identity)

        @staticmethod
        def decode(data):
            operation, identifier, major, minor, patch = struct.unpack("<HIIII", data[0:18])
            if operation != Host.operation_get_identity:
                raise HostException("unexpected operation")
            return Identity(identifier, Version(major, minor, patch))

    def get_identity(self):
        response, response_envelope = self.gateway.rpc(Host.GetIdentity.encode(), self.envelope)
        return Host.GetIdentity.decode(response)

    class ExecutableMetadata:

        def __init__(self, version, length, hash):
            self.version = version
            self.length = length
            self.hash = hash

    class GetExecutableMetadataResult:

        def __init__(self, is_valid, issue=0, metadata=None):
            self.is_valid = is_valid
            self.issue = issue
            self.metadata = metadata

    class GetExecutableMetadata:

        @staticmethod
        def encode():
            return struct.pack("<H", Host.operation_get_executable_metadata)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Host.operation_get_executable_metadata:
                raise HostException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise HostException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Host.GetExecutableMetadataResult(is_valid=False, issue=issue)

            major = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            minor = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            patch = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            length = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            hash = data[index: index + 20]
            metadata = Host.ExecutableMetadata(version=Version(major, minor, patch), length=length, hash=hash)

            return Host.GetExecutableMetadataResult(is_valid=True, metadata=metadata)

    def get_executable_metadata(self):
        response, response_envelope = self.gateway.rpc(Host.GetExecutableMetadata.encode(), self.envelope)
        return Host.GetExecutableMetadata.decode(response)

    class UpdateMetadata:

        def __init__(self, executable_metadata, hash, flags, initialization_vector):
            self.executable_metadata = executable_metadata
            self.hash = hash
            self.flags = flags
            self.initialization_vector = initialization_vector

    class GetUpdateMetadataResult:

        def __init__(self, is_valid, issue=0, metadata=None):
            self.is_valid = is_valid
            self.issue = issue
            self.metadata = metadata

    class GetUpdateMetadata:

        @staticmethod
        def encode():
            return struct.pack("<H", Host.operation_get_update_metadata)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Host.operation_get_update_metadata:
                raise HostException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise HostException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Host.GetExecutableMetadataResult(is_valid=False, issue=issue)

            major = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            minor = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            patch = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            length = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            hash = data[index: index + 20]
            index += 20
            executable_metadata = Host.ExecutableMetadata(Version(major, minor, patch), length, hash)

            hash = data[index: index + 20]
            index += 20
            flags = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            initialization_vector = data[index: index + 16]
            metadata = Host.UpdateMetadata(executable_metadata, hash, flags, initialization_vector)

            return Host.GetExecutableMetadataResult(is_valid=True, metadata=metadata)

    def get_update_metadata(self):
        self.gateway.tx(Host.GetUpdateMetadata.encode(), self.envelope)
        response, response_envelope = self.rx()
        return Host.GetUpdateMetadata.decode(response)

    class UpdateResult:

        def __init__(self, is_valid, issue=0):
            self.is_valid = is_valid
            self.issue = issue

    class Update:

        @staticmethod
        def encode():
            return struct.pack("<H", Host.operation_update)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Host.operation_update:
                raise HostException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise HostException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Host.UpdateResult(is_valid=False, issue=issue)

            return Host.UpdateResult(is_valid=True)

    def update(self):
        self.gateway.tx(Host.Update.encode(), self.envelope)
        response, response_envelope = self.rx()
        return Host.Update.decode(response)

    class ExecuteResult:

        def __init__(self, is_valid, issue=0):
            self.is_valid = is_valid
            self.issue = issue

    class Execute:

        @staticmethod
        def encode():
            return struct.pack("<H", Host.operation_execute)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Host.operation_execute:
                raise HostException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise HostException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Host.ExecuteResult(is_valid=False, issue=issue)

            return Host.ExecuteResult(is_valid=True)

    def execute(self):
        response, response_envelope = self.gateway.rpc(Host.Execute.encode(), self.envelope)
        return Host.Execute.decode(response)

    class GetUpdateStorage:

        @staticmethod
        def encode(location, length):
            return struct.pack("<HII", Host.operation_get_update_storage, location, length)

        @staticmethod
        def decode(data):
            operation = struct.unpack("<H", data)[0]
            return operation

    class UpdateRead:

        @staticmethod
        def encode(location, length, result, code=0, bytes=None):
            data = struct.pack("<HIHB", Host.operation_update_read, location, length, 1 if result else 0)
            if result:
                data += bytes
            else:
                data += struct.pack("<I", code)
            return data

        @staticmethod
        def decode(data):
            operation, location, length = struct.unpack("<HLH", data)
            return operation, location, length

    class StatusProgress:

        @staticmethod
        def decode(data):
            operation, amount = struct.unpack("<Hf", data)
            return operation, amount

    def __init__(self, target, source, system, subsystem):
        self.system = system
        self.subsystem = subsystem
        self.target = target
        self.source = source

        self.update_file = None
        self.gateway = None
        self.envelope = Envelope(
            target=self.target,
            source=self.source,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_request
        )

    def process_get_update_storage_request(self, message, envelope):
        operation = Host.GetUpdateStorage.decode(message)
        location = 0
        length = len(self.update_file)
        response = Host.GetUpdateStorage.encode(location, length)
        response_envelope = Envelope(
            target=self.target,
            source=self.source,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_response
        )
        self.gateway.tx(response, response_envelope)

    def process_update_read_request(self, message, envelope):
        operation, location, length = Host.UpdateRead.decode(message)
        data = self.update_file[location: location + length]
        response = Host.UpdateRead.encode(location, length, True, bytes=data)
        response_envelope = Envelope(
            target=self.target,
            source=self.source,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_response
        )
        self.gateway.tx(response, response_envelope)

    def process_request(self, message, envelope):
        operation = struct.unpack("<H", message[0:2])[0]
        if operation == Host.operation_get_update_storage:
            self.process_get_update_storage_request(message, envelope)
        elif operation == Host.operation_update_read:
            self.process_update_read_request(message, envelope)

    def process_status_progress_event(self, message, envelope):
        operation, amount = Host.StatusProgress.decode(message)
        print(f"progress {amount * 100.0:0.1f}%")

    def process_event(self, message, envelope):
        operation = struct.unpack("<H", message[0:2])[0]
        if operation == Host.operation_status_progress:
            self.process_status_progress_event(message, envelope)

    def rx(self):
        start = time.time()
        while True:
            response = self.gateway.rx_waiting()
            if response is None:
                duration = time.time() - start
                if duration > 0.5:
                    raise HostException("timeout")
                continue
            message, envelope = response
            if envelope.type is Envelope.type_request:
                self.process_request(message, envelope)
            elif envelope.type is Envelope.type_event:
                self.process_event(message, envelope)
            elif envelope.type is Envelope.type_response:
                return response

    def update_and_execute(self, path):
        with open(path, 'rb') as file:
            self.update_file = file.read(-1)

        port = Gateway.find_serial_port(vid=0x0403, pid=0x6001)
        self.gateway = Gateway(port)
        self.gateway.trace = True

        identity = self.get_identity()
        version = identity.version
        print(f"boot loader {identity.identifier:08x} {version.major}.{version.minor}.{version.patch}")

        result = self.update()
        if not result.is_valid:
            print(f"update failed: {result.issue}")
            return
        print(f"update success")

        result = self.execute()
        if not result.is_valid:
            print(f"execute failed: {result.issue}")
            return
        print(f"execute success")


parser = argparse.ArgumentParser(description="Host Boot Loader Update")
parser.add_argument("--update", help="firmware update file (generated by encrypt.py)")
parser.add_argument("--target", help="target to use in envelope", type=int)
parser.add_argument("--source", help="source to use in envelope", type=int)
parser.add_argument("--system", help="system to use in envelope", type=int)
parser.add_argument("--subsystem", help="subsystem to use in envelope", type=int)
parser.set_defaults(update='update.bin', target=0, source=1, system=0, subsystem=0)
args = parser.parse_args()
secure_boot = Host(args.target, args.source, args.system, args.subsystem)
secure_boot.update_and_execute(args.update)
