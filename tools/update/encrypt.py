#!/usr/bin/env python3

import argparse
import binascii
from Crypto.Cipher import AES
import hashlib
from intelhex import IntelHex
import os
import struct
import sys


def int_arg(s):
    return int(s, 0)


def key_arg(s):
    try:
        key = binascii.unhexlify(s)
        if len(key) != 16:
            raise Exception("invalid key length")
        return key
    except Exception:
        raise ValueError("key value expected as a 16-byte hex string")


class Update:

    fd_boot_executable_metadata_header_magic = 0xb001da1a
    fd_boot_executable_metadata_header_version = 1
    fd_boot_executable_footer_magic = 0xb001e00d

    fd_boot_update_metadata_header_magic = 0xab09da1e
    fd_boot_update_metadata_header_version = 1
    fd_boot_update_footer_magic = 0xda1ee00d

    fd_boot_update_metadata_flag_encrypted = 0x0001

    def __init__(self):
        pass

    @staticmethod
    def write_mock(file_name, major, minor, patch, metadata_offset=256):
        magic = Update.fd_boot_executable_metadata_header_magic
        version = Update.fd_boot_executable_metadata_header_version
        length = 0
        hash = bytes(20)
        pad = bytes(16)
        addendum = bytes()

        intelHex = IntelHex()
        for i in range(metadata_offset):
            intelHex[i] = i
        metadata = struct.pack("<LLLLLL20sL16s", magic, version, major, minor, patch, length, hash, len(addendum), pad)
        for i in range(len(metadata)):
            intelHex[metadata_offset + i] = metadata[i]
        for i in range(16):
            intelHex[metadata_offset + len(metadata) + i] = i
        intelHex.write_hex_file(file_name)

    @staticmethod
    def write_mocks():
        key = binascii.unhexlify("000102030405060708090a0b0c0d0e0f")
        Update.write_mock("application_1_2_3.hex", 1, 2, 3)
        Update.package(argparse.Namespace(input="application_1_2_3.hex", output="{base}.bin", key=key, metadata_offset=256))
        Update.write_mock("application_1_2_4.hex", 1, 2, 4)
        Update.package(argparse.Namespace(input="application_1_2_4.hex", output="{base}.bin", key=key, metadata_offset=256))

    @staticmethod
    def replace(collection, start, replacement):
        end = start + len(replacement)
        collection[start:end] = replacement

    @staticmethod
    def sha1_hash(data):
        sha1_fn = hashlib.sha1()
        sha1_fn.update(data)
        return sha1_fn.digest()

    @staticmethod
    def encrypt_data(key, initialization_vector, data):
        aes = AES.new(key, AES.MODE_CBC, IV=initialization_vector)
        return aes.encrypt(bytes(data))

    @staticmethod
    def pad(data, block_size):
        if (len(data) % block_size) == 0:
            return data
        count = block_size - (len(data) % block_size)
        random = os.urandom(count)
        padded_data = data + random
        return padded_data

    @staticmethod
    def load_executable(path):
        intel_hex = IntelHex(path)
        segments = intel_hex.segments()
        address = segments[0][0]
        address_end = segments[-1][1]
        size = address_end - address
        data = intel_hex.tobinarray(start=address, size=size)
        return address, bytearray(data)

    @staticmethod
    def get_executable_metadata(firmware_data, metadata_offset):
        data = firmware_data[metadata_offset:metadata_offset + 64]
        magic, version, major, minor, patch, length, hash, addendum_length, pad = struct.unpack("<LLLLLL20sL16s", data)
        if magic != Update.fd_boot_executable_metadata_header_magic:
            raise Exception("executable header magic incorrect")
        if version != Update.fd_boot_executable_metadata_header_version:
            raise Exception("executable header version incorrect")
        if length != 0:
            raise Exception("executable length incorrect")
        if hash != bytes(20):
            raise Exception("executable hash incorrect")
        if addendum_length != 0:
            raise Exception("executable addendum length incorrect")
        return major, minor, patch

    @staticmethod
    def package_executable(data, metadata_offset):
        data = Update.pad(data + bytes(4), 64)
        data[len(data) - 4:] = bytearray(struct.pack("<L", Update.fd_boot_executable_footer_magic))
        length_offset = 20
        Update.replace(data, metadata_offset + length_offset, struct.pack("<L", len(data)))
        hash = Update.sha1_hash(data)
        hash_offset = 24
        Update.replace(data, metadata_offset + hash_offset, hash)
        return data, hash

    @staticmethod
    def package_update(data):
        data += struct.pack("<60sL", os.urandom(60), Update.fd_boot_update_footer_magic)
        hash = Update.sha1_hash(data)
        hash_offset = 44
        Update.replace(data, hash_offset, hash)
        return data

    @staticmethod
    def package(args):
        addendum = bytes()

        print(f"reading firmware {args.input}")

        executable_address, executable_data = Update.load_executable(args.input)
        if hasattr(args, "address") and (args.address != executable_address):
            raise Exception(f"firmware address 0x{executable_address:08X} mismatch, expected 0x{args.address:08X}")
        major, minor, patch = Update.get_executable_metadata(executable_data, args.metadata_offset)
        print(f"found: version {major}.{minor}.{patch}, {len(executable_data)} bytes at 0x{executable_address:08X}")
        executable, executable_hash = Update.package_executable(executable_data, args.metadata_offset)

        initialization_vector = os.urandom(16)
        encrypted_executable = Update.encrypt_data(args.key, initialization_vector, executable)

        metadata = struct.pack(
            "<LLLLLL20s20sL16sL40s",
            Update.fd_boot_update_metadata_header_magic,
            Update.fd_boot_update_metadata_header_version,
            major,
            minor,
            patch,
            len(executable),
            executable_hash,
            bytes(20),
            Update.fd_boot_update_metadata_flag_encrypted,
            initialization_vector,
            len(addendum),
            bytes(40)
        )

        encrypted_metadata = Update.encrypt_data(args.key, initialization_vector, metadata)

        update = Update.package_update(bytearray(metadata + encrypted_metadata + encrypted_executable))

        base = os.path.splitext(args.input)[0]
        output = args.output.format(base=base, major=major, minor=minor, patch=patch)
        print(f"writing update to {output}")
        with open(output, "wb") as f:
            f.write(update)

    @staticmethod
    def main(argv):
        parser = argparse.ArgumentParser(description='Create a firmware update binary.')
        parser.add_argument('--key', type=key_arg, required=True)
        parser.add_argument('--input', required=True)
        parser.add_argument('--output', default="{base}_{major}_{minor}_{patch}.bin")
        parser.add_argument('--metadata_offset', type=int_arg, default=256)
        parser.add_argument('--address', type=int_arg)
        args = parser.parse_args(argv)
        try:
            Update.package(args)
        except Exception as e:
            print(f"error: {str(e)}")


#Update.write_mocks()
Update.main(sys.argv[1:])
