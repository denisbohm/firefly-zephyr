set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

zephyr_sources(src/fd_swd.c)
zephyr_sources(src/fd_swd_channel.c)
