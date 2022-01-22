#!/usr/bin/env python3

import binascii
from Crypto.Cipher import AES
from collections import namedtuple
import hashlib
from intelhex import IntelHex
import os
import struct
import sys


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
    def write_mock(metadata_offset=256):
        magic = Update.fd_boot_executable_metadata_header_magic
        version = Update.fd_boot_executable_metadata_header_version
        major = 1
        minor = 2
        patch = 3
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
        intelHex.write_hex_file("application.hex")

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
        data += struct.pack("<60sL", os.urandom(60), Update.fd_boot_executable_footer_magic)
        hash = Update.sha1_hash(data)
        hash_offset = 44
        Update.replace(data, hash_offset, hash)
        return data

    @staticmethod
    def package(executable_path, key, encrypted_executable_path, metadata_offset=256):
        addendum = bytes()

        executable_address, executable_data = Update.load_executable(executable_path)
        major, minor, patch = Update.get_executable_metadata(executable_data, metadata_offset)
        print(f"encrypting {executable_path} {major}.{minor}.{patch}: {len(executable_data)} bytes at 0x{executable_address:08X}")
        executable, executable_hash = Update.package_executable(executable_data, metadata_offset)

        initialization_vector = os.urandom(16)
        encrypted_executable = Update.encrypt_data(key, initialization_vector, executable)

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

        encrypted_metadata = Update.encrypt_data(key, initialization_vector, metadata)

        update = Update.package_update(bytearray(metadata + encrypted_metadata + encrypted_executable))

        print("writing " + encrypted_executable_path)
        with open(encrypted_executable_path, "wb") as f:
            f.write(update)

    @staticmethod
    def main(argv):
        executable_path = "application.hex"
        encrypted_executable_path = "application.bin"

        key = binascii.unhexlify("000102030405060708090a0b0c0d0e0f")

        version_major = 0
        version_minor = 0
        version_patch = 0
        version_commit = binascii.unhexlify("000102030405060708090a0b0c0d0e0f10111213")

        comment = "testing 1... 2... 3..."

        addendum = "{\"testing\": true}"

        i = 1
        while i < len(argv):
            arg = argv[i]
            if arg == "-executable":
                i += 1
                executable_path = argv[i]
            elif arg == "-encrypted-executable":
                i += 1
                encrypted_executable_path = argv[i]
            elif arg == "-key":
                i += 1
                key = binascii.unhexlify(argv[i])
            elif arg == "-version":
                i += 1
                version_major = int(argv[i])
                i += 1
                version_minor = int(argv[i])
                i += 1
                version_patch = int(argv[i])
                i += 1
                version_commit = binascii.unhexlify(argv[i])
            elif arg == "-comment":
                i += 1
                comment = argv[i]
            elif arg == "-addendum":
                i += 1
                addendum = argv[i]
            else:
                print("unexpected argument")
            i += 1

        Update.package(executable_path, key, encrypted_executable_path)


Update.write_mock()
Update.main(sys.argv)
