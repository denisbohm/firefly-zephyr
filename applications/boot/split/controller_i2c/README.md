# Split Boot Loader: Controller
This application shows how to use the controller side of the split boot loader on an nRF5340DK board.

1. Build the 'blinky' application and create an update C source files (update.c and update.h).  See applications/boot/blinky for details.

1. Connect the controller nRF5340DK board I2C SCL pins P1.02 and the I2C SDA pin P1.03 to the peripheral board.

1. Reset the board running the 'peripheral' with no application firmware installed.

1. Run this 'controller' side of the boot loader on the nRF5340DK board.  It will install the update and start the application firmware running.
