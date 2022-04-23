set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

if(CONFIG_FIREFLY_SUBSYS_BLE)
    zephyr_sources(src/fd_ble.c)
    zephyr_sources(src/fd_ble_dis.c)
    zephyr_sources(src/fd_ble_l2cap.c)
endif()
