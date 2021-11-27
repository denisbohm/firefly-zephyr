set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

if(CONFIG_FIREFLY_SUBSYS_USB)
    zephyr_sources(src/fd_usb.c)
endif()

if(CONFIG_FIREFLY_SUBSYS_USB_CDC)
    zephyr_sources(src/fd_usb_cdc.c)
endif()

if(CONFIG_FIREFLY_SUBSYS_USB_MSC)
    zephyr_sources(src/fd_usb_msc.c)
endif()
