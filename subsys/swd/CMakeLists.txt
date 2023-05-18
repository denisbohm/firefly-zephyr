set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_FIREFLY_SUBSYS_SWD src/fd_swd.c)
zephyr_sources_ifdef(CONFIG_FIREFLY_SUBSYS_SWD src/fd_swd_channel.c)
