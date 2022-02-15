# Split Boot Loader: Peripheral
This application shows how to use the peripheral side of the split boot loader on an nRF5340DK board.

1. Build the 'blinky' application and create an update binary.  See applications/boot/blinky for details.

1. Run 'nrfjprog --recover' to erase any existing firmware.

1. Run the 'peripheral' boot loader.  If there is valid firmware then the boot loader will start it running.  If there is no valid firmware then the boot loader will open the serial port to accept an update.

1. Run the tools/update/controller.py script to update the firmware.  After the update is complete the firmware will start.
