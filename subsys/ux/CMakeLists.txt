set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

if(CONFIG_FIREFLY_SUBSYS_UX)
    zephyr_sources(src/fd_button.c)
    zephyr_sources(src/fd_ux.c)
    zephyr_sources(src/fd_ux_button.c)
    zephyr_sources(src/fd_ux_touch.c)
endif()