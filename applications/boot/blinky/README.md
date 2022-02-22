# Blinky
The blinky application blinks LED4 on an nRF5340DK.  It is setup to run from the boot loader, not standalone.  See the boot application for more info.

## Build
To build the blinky application and generate an update binary :
```
$ west build
$ ./update.py
```

The update binary will be in the file 'update.bin'.

Note that this build uses a custom linker script to place executable at a known location so that the boot loader can find it.  This custom linker script is a modified copy of the one in nRF Connect SDK 1.9.0.  If you are using a newer version, you may need to update it.
