cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf5340dk_nrf5340_cpuapp)

find_package(Zephyr)

project(watch)

set(CMAKE_BUILD_TYPE Debug)

target_sources(app PRIVATE
    src/fd_watch.c
    src/fd_watch_screen_powering_off.c
    src/fd_watch_screen_splash.c
    src/fd_watch_screen_time.c
    src/main.c
)
