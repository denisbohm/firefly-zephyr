set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

zephyr_sources(src/fd_ux.c)
zephyr_sources(src/fd_ux_button.c)
