#pragma once

#include <stdint.h>

/*
 * Product-specified custom-board map. GPIO 30/31/32 require the B package and all
 * assignments still require schematic/electrical verification before energizing a board.
 */
#define RASTER_WS2812_CHANNEL_1_PIN 0u
#define RASTER_WS2812_CHANNEL_2_PIN 1u
#define RASTER_WS2812_CHANNEL_3_PIN 2u
#define RASTER_WS2812_CHANNEL_4_PIN 3u
#define RASTER_WS2812_CHANNEL_5_PIN 4u
#define RASTER_WS2812_CHANNEL_6_PIN 5u
#define RASTER_WS2812_CHANNEL_7_PIN 6u
#define RASTER_WS2812_CHANNEL_8_PIN 7u
#define RASTER_WS2812_CHANNEL_9_PIN 8u
#define RASTER_WS2812_CHANNEL_10_PIN 9u

#define RASTER_W5500_MISO_PIN 16u
#define RASTER_W5500_CS_PIN 17u
#define RASTER_W5500_SCLK_PIN 18u
#define RASTER_W5500_MOSI_PIN 19u
#define RASTER_W5500_RESET_PIN 30u
#define RASTER_W5500_INTERRUPT_PIN 31u

#define RASTER_OLED_SDA_PIN 20u
#define RASTER_OLED_SCL_PIN 21u
#define RASTER_USER_BUTTON_PIN 26u
#define RASTER_STATUS_LED_PIN 28u
#define RASTER_BUZZER_PIN 32u

#define RASTER_LED_CHANNELS 10u
