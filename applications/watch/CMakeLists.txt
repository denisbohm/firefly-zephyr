cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf5340dk_nrf5340_cpuapp)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH FIREFLY_ZEPHYR_APPLICATIONS_DIR)
cmake_path(GET FIREFLY_ZEPHYR_APPLICATIONS_DIR PARENT_PATH FIREFLY_ZEPHYR_DIR)
set(ZEPHYR_EXTRA_MODULES ${FIREFLY_ZEPHYR_DIR})
MESSAGE(STATUS "ZEPHYR_EXTRA_MODULES=${ZEPHYR_EXTRA_MODULES}")

find_package(Zephyr)

project(watch)

set(PM_STATIC_YML_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_BOARD}_pm_debug.yml")
MESSAGE(STATUS "PM_STATIC_YML_FILE=${PM_STATIC_YML_FILE}")

set(CMAKE_BUILD_TYPE Debug)

target_sources(app PRIVATE
    src/fd_watch.c
    src/fd_watch_screen_powering_off.c
    src/fd_watch_screen_splash.c
    src/fd_watch_screen_time.c
    src/main.c
)
