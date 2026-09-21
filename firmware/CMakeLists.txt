add_executable(raster_pick_to_light
    main.c
    application/channel_manager.c
    application/color_manager.c
    application/pharmacy_protocol.c
    application/shelf_manager.c
    configuration/config_manager.c
    configuration/flash_storage.c
    drivers/button.c
    drivers/buzzer.c
    drivers/oled.c
    drivers/sh1106g.c
    drivers/oled_manager.c
    drivers/status_led.c
    drivers/ws2812.c
    network/http_server.c
    network/w5500_eth.c
    ota/ota_state.c
    ota/ota_verify.c
    system/raster_runtime.c
    web/dashboard_assets.c
)

target_include_directories(raster_pick_to_light
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/application
        ${CMAKE_CURRENT_LIST_DIR}/configuration
        ${CMAKE_CURRENT_LIST_DIR}/drivers
        ${CMAKE_CURRENT_LIST_DIR}/network
        ${CMAKE_CURRENT_LIST_DIR}/ota
        ${CMAKE_CURRENT_LIST_DIR}/system
        ${CMAKE_CURRENT_LIST_DIR}/web
)

target_compile_features(raster_pick_to_light PRIVATE c_std_11)
target_compile_options(raster_pick_to_light PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(raster_pick_to_light PRIVATE
    PICO_BUILD=1
    RASTER_ENABLE_WS2812_CHAIN=1
    RASTER_ENABLE_ETHERNET=1
)

pico_generate_pio_header(raster_pick_to_light
    ${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812.pio
)

target_link_libraries(raster_pick_to_light
    PRIVATE
        hardware_dma
        hardware_i2c
        hardware_pio
        hardware_spi
        hardware_watchdog
        pico_stdlib
)

pico_enable_stdio_usb(raster_pick_to_light 0)
pico_enable_stdio_uart(raster_pick_to_light 1)
pico_add_extra_outputs(raster_pick_to_light)
