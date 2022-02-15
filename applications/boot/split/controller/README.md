# Split Boot Loader: Controller
This application shows how to use the controller side of the split boot loader on an nRF5340DK board.

1. Build the 'blinky' application and create an update C source files (update.c and update.h).  See applications/boot/blinky for details.

1. Connect two nRF5340DK boards with the pins P?.?? and P?.?? crossed over.

1. Reset the nRF5340DK board running the 'peripheral' with no application firmware installed.

1. Run this 'controller' side of the boot loader on the other nRF5340DK.  It will install the update and start the application firmware running.
