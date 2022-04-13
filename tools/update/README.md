# Firmware Update
This repository contains a set of utilities for firmware updates.

## Update Packaging

The encrypt.py script is used to package an update.  The firmware is encrypted and metadata is added to create the update file for the boot loader.

```
$ ./encrypt.py 
usage: encrypt.py [-h] --key KEY --input INPUT [--output OUTPUT] [--type {binary,c_binary,c,none}]
                  [--metadata_offset METADATA_OFFSET] [--address ADDRESS]
```

### key
The key is an AES-128 key passed as a hex string (for example "000102030405060708090a0b0c0d0e0f").

## input
The input is the firmware file which must be in Intel Hex format.

## output (default {base}_{major}_{minor}_{patch}.bin)
The output is the packaged update file.  The default is to derive the file name from the input name while adding the version number.

## type (default binary)
The type is the format of the packaged update file: binary, c_binary, c, or none.
### binary
This is a binary file that can be stored in the file system, sent over the network, etc.
### c_binary
This is a binary encoded as a C language byte array.  It is used when you want to embed the update into other firmware.  For example, if you have hardware with a main processor and some small sensor processors.  The main processor can embed firmware updates for the sensor processors and apply them during startup if the sensors are out of date.
### c
This is just a raw unencrypted C language byte array.  It does not have any metadata embeded in it and is not encrypted.

## metadata_offset (default 256)
There is some metadata about the firmware stored in the firmware image itself.  This is typically located after the interrupt vectors.  The metadata offset used must be provided so that it can be filled in by the packager.

## address
This is the address where the firmware starts.  It is used as a check to ensure the input matches.