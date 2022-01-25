cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf5340dk_nrf5340_cpuapp)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(blinky)

set(CMAKE_BUILD_TYPE Debug)

target_sources(app PRIVATE
    src/main.c
    src/executable_metadata.c
)

zephyr_code_relocate(src/executable_metadata.c FLASH)

zephyr_linker_sources(SECTIONS custom-sections.ld)
