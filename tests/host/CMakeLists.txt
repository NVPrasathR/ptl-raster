cmake_minimum_required(VERSION 3.19)

project(raster_host_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCCompilerFlag)
enable_testing()

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(FIRMWARE_DIR "${REPO_ROOT}/firmware")

add_library(raster_firmware_host
    "${FIRMWARE_DIR}/application/channel_manager.c"
    "${FIRMWARE_DIR}/application/color_manager.c"
    "${FIRMWARE_DIR}/application/pharmacy_protocol.c"
    "${FIRMWARE_DIR}/application/shelf_manager.c"
    "${FIRMWARE_DIR}/configuration/config_manager.c"
    "${FIRMWARE_DIR}/configuration/flash_storage.c"
    "${FIRMWARE_DIR}/drivers/ws2812.c"
    "${FIRMWARE_DIR}/network/http_server.c"
    "${FIRMWARE_DIR}/network/w5500_eth.c"
    "${FIRMWARE_DIR}/ota/host_verify_backend.c"
    "${FIRMWARE_DIR}/ota/ota_state.c"
    "${FIRMWARE_DIR}/ota/ota_verify.c"
    "${FIRMWARE_DIR}/web/dashboard_assets.c"
)

target_include_directories(raster_firmware_host
    PUBLIC
        "${FIRMWARE_DIR}"
        "${FIRMWARE_DIR}/application"
        "${FIRMWARE_DIR}/configuration"
        "${FIRMWARE_DIR}/drivers"
        "${FIRMWARE_DIR}/network"
        "${FIRMWARE_DIR}/ota"
        "${FIRMWARE_DIR}/web"
)

target_compile_options(raster_firmware_host PRIVATE
    -Wall
    -Wextra
    -Werror
    -include
    strings.h
)

check_c_compiler_flag("-fsanitize=address,undefined" RASTER_HAS_ASAN_UBSAN)
if(RASTER_HAS_ASAN_UBSAN AND NOT CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(raster_firmware_host PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(raster_firmware_host PUBLIC -fsanitize=address,undefined)
endif()

add_executable(raster_host_unit_tests test_host.c)
target_link_libraries(raster_host_unit_tests PRIVATE raster_firmware_host)
target_compile_options(raster_host_unit_tests PRIVATE -Wall -Wextra -Werror)

add_executable(raster_dashboard_endpoint_test test_dashboard_endpoint.c)
target_link_libraries(raster_dashboard_endpoint_test PRIVATE raster_firmware_host)
target_compile_options(raster_dashboard_endpoint_test PRIVATE -Wall -Wextra -Werror)

add_executable(raster_legacy_leds_test test_legacy_leds.c)
target_link_libraries(raster_legacy_leds_test PRIVATE raster_firmware_host)
target_compile_options(raster_legacy_leds_test PRIVATE -Wall -Wextra -Werror)

add_test(NAME raster_host_unit_tests COMMAND raster_host_unit_tests)
add_test(NAME raster_dashboard_endpoint_test COMMAND raster_dashboard_endpoint_test)
add_test(NAME raster_legacy_leds_test COMMAND raster_legacy_leds_test)
