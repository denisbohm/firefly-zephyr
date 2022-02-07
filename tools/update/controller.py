#!/usr/bin/env python3

from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway
from firefly.transport.gateway import GatewayException

import struct
import time
import argparse


class Version:

    def __init__(self, major, minor, patch):
        self.major = major
        self.minor = minor
        self.patch = patch


class Identity:

    def __init__(self, version, identifier):
        self.version = version
        self.identifier = identifier


class ControllerException(Exception):
    pass


class Controller:

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
            return struct.pack("<H", Controller.operation_get_identity)

        @staticmethod
        def decode(data):
            operation, major, minor, patch, length = struct.unpack("<HIIIB", data[0:15])
            if operation != Controller.operation_get_identity:
                raise ControllerException("unexpected operation")
            identifier = data[15:15 + length].decode("utf-8")
            return Identity(Version(major, minor, patch), identifier)

    def get_identity(self):
        response, response_envelope = self.gateway.rpc(Controller.GetIdentity.encode(), self.envelope)
        return Controller.GetIdentity.decode(response)

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
            return struct.pack("<H", Controller.operation_get_executable_metadata)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Controller.operation_get_executable_metadata:
                raise ControllerException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise ControllerException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Controller.GetExecutableMetadataResult(is_valid=False, issue=issue)

            major = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            minor = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            patch = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            length = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            hash = data[index: index + 20]
            metadata = Controller.ExecutableMetadata(version=Version(major, minor, patch), length=length, hash=hash)

            return Controller.GetExecutableMetadataResult(is_valid=True, metadata=metadata)

    def get_executable_metadata(self):
        response, response_envelope = self.gateway.rpc(Controller.GetExecutableMetadata.encode(), self.envelope)
        return Controller.GetExecutableMetadata.decode(response)

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
            return struct.pack("<H", Controller.operation_get_update_metadata)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Controller.operation_get_update_metadata:
                raise ControllerException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise ControllerException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Controller.GetExecutableMetadataResult(is_valid=False, issue=issue)

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
            executable_metadata = Controller.ExecutableMetadata(Version(major, minor, patch), length, hash)

            hash = data[index: index + 20]
            index += 20
            flags = struct.unpack("<I", data[index: index + 4])[0]
            index += 4
            initialization_vector = data[index: index + 16]
            metadata = Controller.UpdateMetadata(executable_metadata, hash, flags, initialization_vector)

            return Controller.GetExecutableMetadataResult(is_valid=True, metadata=metadata)

    def get_update_metadata(self):
        self.gateway.tx(Controller.GetUpdateMetadata.encode(), self.envelope)
        response, response_envelope = self.rx()
        return Controller.GetUpdateMetadata.decode(response)

    class UpdateResult:

        def __init__(self, is_valid, issue=0):
            self.is_valid = is_valid
            self.issue = issue

    class Update:

        @staticmethod
        def encode():
            return struct.pack("<H", Controller.operation_update)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Controller.operation_update:
                raise ControllerException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise ControllerException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Controller.UpdateResult(is_valid=False, issue=issue)

            return Controller.UpdateResult(is_valid=True)

    def update(self):
        self.gateway.tx(Controller.Update.encode(), self.envelope)
        response, response_envelope = self.rx()
        return Controller.Update.decode(response)

    class ExecuteResult:

        def __init__(self, is_valid, issue=0):
            self.is_valid = is_valid
            self.issue = issue

    class Execute:

        @staticmethod
        def encode():
            return struct.pack("<H", Controller.operation_execute)

        @staticmethod
        def decode(data):
            operation, success = struct.unpack("<HB", data[0:3])
            index = 3
            if operation != Controller.operation_execute:
                raise ControllerException("unexpected operation")
            if success == 0:
                code = struct.unpack("<I", data[index: index + 4])[0]
                raise ControllerException(f"operation failed: {code}")
            is_valid = data[index] != 0
            index += 1
            if not is_valid:
                issue = data[index]
                return Controller.ExecuteResult(is_valid=False, issue=issue)

            return Controller.ExecuteResult(is_valid=True)

    def execute(self):
        response, response_envelope = self.gateway.rpc(Controller.Execute.encode(), self.envelope)
        return Controller.Execute.decode(response)

    class GetUpdateStorage:

        @staticmethod
        def encode(location, length):
            return struct.pack("<HII", Controller.operation_get_update_storage, location, length)

        @staticmethod
        def decode(data):
            operation = struct.unpack("<H", data)[0]
            return operation

    class UpdateRead:

        @staticmethod
        def encode(location, length, result, code=0, bytes=None):
            data = struct.pack("<HIHB", Controller.operation_update_read, location, length, 1 if result else 0)
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
        operation = Controller.GetUpdateStorage.decode(message)
        location = 0
        length = len(self.update_file)
        response = Controller.GetUpdateStorage.encode(location, length)
        response_envelope = Envelope(
            target=self.target,
            source=self.source,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_response
        )
        self.gateway.tx(response, response_envelope)

    def process_update_read_request(self, message, envelope):
        operation, location, length = Controller.UpdateRead.decode(message)
        data = self.update_file[location: location + length]
        response = Controller.UpdateRead.encode(location, length, True, bytes=data)
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
        if operation == Controller.operation_get_update_storage:
            self.process_get_update_storage_request(message, envelope)
        elif operation == Controller.operation_update_read:
            self.process_update_read_request(message, envelope)

    def process_status_progress_event(self, message, envelope):
        operation, amount = Controller.StatusProgress.decode(message)
        print(f"progress {amount * 100.0:0.1f}%")

    def process_event(self, message, envelope):
        operation = struct.unpack("<H", message[0:2])[0]
        if operation == Controller.operation_status_progress:
            self.process_status_progress_event(message, envelope)

    def rx(self):
        start = time.time()
        while True:
            response = self.gateway.rx_waiting()
            if response is None:
                duration = time.time() - start
                if duration > 1000:
                    raise ControllerException("timeout")
                continue
            start = time.time()
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

        port = Gateway.find_serial_port(vid=0x1366, pid=0x1055)  # nRf5340 DK J-Link
        self.gateway = Gateway(port)
        self.gateway.trace = True

        identity = self.get_identity()
        version = identity.version
        print(f"identity: {identity.identifier} {version.major}.{version.minor}.{version.patch}")

        result = self.update()
        if not result.is_valid:
            print(f"update failed: {result.issue}")
            return
        print(f"update success")

        try:
            result = self.execute()
            if not result.is_valid:
                print(f"execute failed: {result.issue}")
                return
            print(f"execute success")
        except GatewayException:
            print("firmware is running")


parser = argparse.ArgumentParser(description="Boot Split Controller Serial Example")
parser.add_argument("--update", help="firmware update file (generated by encrypt.py)")
parser.add_argument("--target", help="target to use in envelope", type=int)
parser.add_argument("--source", help="source to use in envelope", type=int)
parser.add_argument("--system", help="system to use in envelope", type=int)
parser.add_argument("--subsystem", help="subsystem to use in envelope", type=int)
parser.set_defaults(update='update.bin', target=0, source=1, system=0, subsystem=0)
args = parser.parse_args()
secure_boot = Controller(args.target, args.source, args.system, args.subsystem)
secure_boot.update_and_execute(args.update)
