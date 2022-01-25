cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf5340dk_nrf5340_cpuapp)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH FIREFLY_ZEPHYR_APPLICATIONS_DIR)
cmake_path(GET FIREFLY_ZEPHYR_APPLICATIONS_DIR PARENT_PATH FIREFLY_ZEPHYR_DIR)
set(ZEPHYR_EXTRA_MODULES ${FIREFLY_ZEPHYR_DIR})

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(boot)

set(CMAKE_BUILD_TYPE Debug)

target_sources(app PRIVATE
    src/main.c
    ../../subsys/boot/src/fd_boot_zephyr.c
    ../../subsys/usb/src/fd_usb_msc.c
)
