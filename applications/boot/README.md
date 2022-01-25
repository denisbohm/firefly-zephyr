# Boot
This application shows how to use the firefly boot loader on an nRF5340DK board.

1. Build the 'blinky' application and create an update binary.

1. Run the 'boot' boot loader.  If there is no valid firmware and no valid update then it will mount as a USB MSC device.  Copy the blinky update binary to the USB MSC device.

1. Restart the 'boot' boot loader.  It will install the blinky update and then execute blinky.  LED4 on the nRF5340DK will start blinking.

1. Hold down BUTTON_1 while starting the boot loader to mount as a USB MSC device - even when there is a valid application to run.  This will allow a subsequent update to be placed into the file system.  Restart the boot loader to install and run that update.

Note that CMakeLists.txt is including some of the subsys sources directly.  This is a workaround because compiling subsys sources that use other modules (FATFS, etc) has been problematic...  To reproduce:

1. Comment out the two target_sources subsys lines in CMakeLists.txt.

1. Uncomment the two prj.conf lines CONFIG_FIREFLY_SUBSYS_BOOT_ZEPHYR and CONFIG_FIREFLY_SUBSYS_USB_MSC.

1. A west build will fail compiling those same two target sources.