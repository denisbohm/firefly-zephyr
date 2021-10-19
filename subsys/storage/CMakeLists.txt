set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

zephyr_sources(src/fd_sdcard.c)
zephyr_sources(src/fd_sdcard_spi_zephyr.c)
