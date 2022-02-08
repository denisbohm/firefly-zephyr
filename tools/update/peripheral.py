#!/usr/bin/env python3

from firefly.transport.envelope import Envelope
from firefly.transport.gateway import Gateway

import struct
import argparse


class PeripheralException(Exception):
    pass


class Peripheral:

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
        def decode(data):
            operation = struct.unpack("<H", data)[0]
            if operation != Peripheral.operation_get_identity:
                raise PeripheralException("unexpected operation")

        @staticmethod
        def encode(major, minor, patch, identifier):
            operation = Peripheral.operation_get_identity
            identifier_bytes = identifier.encode('utf-8')
            data = struct.pack("<HIIIB", operation, major, minor, patch, len(identifier_bytes))
            data += identifier_bytes
            return data

    class Update:

        @staticmethod
        def decode(data):
            operation = struct.unpack("<H", data)[0]
            if operation != Peripheral.operation_update:
                raise PeripheralException("unexpected operation")

        @staticmethod
        def encode(success, code=0, is_valid=False, issue=0):
            operation = Peripheral.operation_update
            data = struct.pack("<HB", operation, 1 if success else 0)
            if not success:
                data += struct.pack("<I", code)
            else:
                data += struct.pack("<B", 1 if is_valid else 0)
                if not is_valid:
                    data += struct.pack("<B", issue)
            return data

    class Execute:

        @staticmethod
        def decode(data):
            operation = struct.unpack("<H", data)[0]
            if operation != Peripheral.operation_execute:
                raise PeripheralException("unexpected operation")

        @staticmethod
        def encode(success, code=0, is_valid=False, issue=0):
            operation = Peripheral.operation_execute
            data = struct.pack("<HB", operation, 1 if success else 0)
            if not success:
                data += struct.pack("<I", code)
            else:
                data += struct.pack("<B", 1 if is_valid else 0)
                if not is_valid:
                    data += struct.pack("<B", issue)
            return data

    class GetUpdateStorage:

        @staticmethod
        def encode():
            return struct.pack("<H", Peripheral.operation_get_update_storage)

        @staticmethod
        def decode(data):
            operation, location, length = struct.unpack("<HII", data)
            if operation != Peripheral.operation_get_update_storage:
                raise PeripheralException("unexpected operation")
            return location, length

    class UpdateRead:

        @staticmethod
        def encode(location, length):
            return struct.pack("<HLH", Peripheral.operation_update_read, location, length)

        @staticmethod
        def decode(data):
            operation, location, length, success = struct.unpack("<HLHB", data[0:9])
            if operation != Peripheral.operation_update_read:
                raise PeripheralException("unexpected operation")
            if success == 0:
                raise PeripheralException("update read failed")
            return data[9:9 + length]

    class StatusProgress:

        @staticmethod
        def encode(amount):
            return struct.pack("<Hf", Peripheral.operation_status_progress, amount)

    def __init__(self, target, source, system, subsystem):
        self.system = system
        self.subsystem = subsystem
        self.target = target
        self.source = source

        self.update_file = None
        self.gateway = None
        self.event_envelope = Envelope(
            target=self.source,
            source=self.target,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_event
        )
        self.request_envelope = Envelope(
            target=self.target,
            source=self.source,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_request
        )
        self.response_envelope = Envelope(
            target=self.source,
            source=self.target,
            system=self.system,
            subsystem=self.subsystem,
            type=Envelope.type_response
        )

    def test(self):
        port = Gateway.find_serial_port(vid=0x1366, pid=0x1055)  # nRf5340 DK J-Link
        port = "/dev/tty.usbmodem0009601117525"
        self.gateway = Gateway(port)
        self.gateway.trace = True

        print("waiting for boot split controller to get identity...")

        message, envelope = self.gateway.rx()
        Peripheral.GetIdentity.decode(message)
        self.gateway.tx(Peripheral.GetIdentity.encode(1, 2, 3, "fd_boot"), self.response_envelope)

        print("waiting for boot split controller to request an update...")

        message, envelope = self.gateway.rx()
        Peripheral.Update.decode(message)

        self.gateway.tx(Peripheral.GetUpdateStorage.encode(), self.request_envelope)
        print("waiting for boot split controller to respond to get update storage...")
        message, envelope = self.gateway.rx()
        location, length = Peripheral.GetUpdateStorage.decode(message)
        print(f"get update storage: {location} {length}")

        self.gateway.tx(Peripheral.StatusProgress.encode(0.5), self.event_envelope)

        location = 0
        length = 16
        self.gateway.tx(Peripheral.UpdateRead.encode(location, length), self.request_envelope)
        print("waiting for boot split controller to respond to update read...")
        message, envelope = self.gateway.rx()
        data = Peripheral.UpdateRead.decode(message)
        print(f"update read: {data}")

        self.gateway.tx(Peripheral.Update.encode(success=True, is_valid=True), self.response_envelope)

        print("waiting for boot split controller to request execute...")
        message, envelope = self.gateway.rx()
        Peripheral.Execute.decode(message)

        print("test complete")


parser = argparse.ArgumentParser(description="Boot Split Peripheral Serial Test")
parser.add_argument("--target", help="target to use in envelope", type=int)
parser.add_argument("--source", help="source to use in envelope", type=int)
parser.add_argument("--system", help="system to use in envelope", type=int)
parser.add_argument("--subsystem", help="subsystem to use in envelope", type=int)
parser.set_defaults(target=1, source=0, system=0, subsystem=0)
args = parser.parse_args()
peripheral = Peripheral(args.target, args.source, args.system, args.subsystem)
peripheral.test()
