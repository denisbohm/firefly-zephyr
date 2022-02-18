# Split Boot Loader

The split boot loader is intended for sitations where the device to be securely updated does not have local storage for the packaged secure update binary.  The device to be updated is the 'peripheral'.  It contains all of the boot loader except for the update storage.  The device that is providing the update storage is the 'controller'.

There is one implementation provided for the peripheral using a serial connection.  It has been tested on an nRF5340DK board.

There are two implementations provided for the controller using a serial connection: an MCU based controller (see applications/boot/split/controller), and a laptop based controller (see tools/update/controller.py).