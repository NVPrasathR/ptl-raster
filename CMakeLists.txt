cmake_minimum_required(VERSION 3.19)

if(DEFINED ENV{PICO_SDK_PATH})
    set(RASTER_PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
elseif(DEFINED PICO_SDK_PATH)
    set(RASTER_PICO_SDK_PATH "${PICO_SDK_PATH}")
else()
    set(RASTER_PICO_SDK_PATH "/System/Volumes/Data/private/tmp/pico-sdk")
endif()

if(NOT EXISTS "${RASTER_PICO_SDK_PATH}/pico_sdk_init.cmake")
    message(FATAL_ERROR
        "Pico SDK not found at '${RASTER_PICO_SDK_PATH}'. Set PICO_SDK_PATH or install the SDK before configuring this project."
    )
endif()

set(PICO_SDK_PATH "${RASTER_PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)
set(PICO_BOARD raster_custom CACHE STRING "Target board definition" FORCE)
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE STRING "Additional board header paths" FORCE)
if(APPLE AND EXISTS "/opt/homebrew/bin/arm-none-eabi-gcc")
    set(PICO_TOOLCHAIN_PATH "/opt/homebrew" CACHE PATH "Compiler search path")
endif()

include("${PICO_SDK_PATH}/pico_sdk_init.cmake")

project(raster_pick_to_light LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_subdirectory(firmware)
