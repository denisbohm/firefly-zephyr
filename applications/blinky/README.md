# Blinky
The blinky application blinks LED4 on an nRF5340DK.  It is setup to run from the boot loader, not standalone.  See the boot application for more info.

## Build
To build the blinky application and generate an update binary :
```
$ west build
$ source update.py
```

The update binary will be in the file 'update.bin'.

Note that this build uses a custom linker script to place executable at a known location so that the boot loader can find it.  This custom linker script is a modified copy of the one in nRF Connect SDK 1.8.0.  If you are using a newer version, you may need to update it.  

CONFIG_CODE_DATA_RELOCATION is probably the way to go instead of a custom linker.  However, getting that setup and running correctly has been problematic...  To reproduce:

1. Uncomment the last two lines in CMakeLists.txt.

1. Uncomment CONFIG_CODE_DATA_RELOCATION, then comment out CONFIG_FLASH_BASE_ADDRESS and CONFIG_HAVE_CUSTOM_LINKER_SCRIPT in prj.conf.

1. Comment out the __attribute__ in src/executable_metadata.c.

1. Change ".executable_metadata_section" to ".executable_metadata_section.*" in custom-sections.ld.

1. Run in the debugger.  There will be a fault reset loop.  The fault happens in data_copy_xip_relocation.

I think the issue is that it is trying to relocate the section by copying it in flash, which obviously doesn't work.  The goal is to locate the executable_metadata at a specific address after the interrupt vectors.  Maybe relocate can't do that.  But then what is the Zephyr way to do that?