#ifndef RASTER_WS2812_H
#define RASTER_WS2812_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WS2812_CHANNEL_COUNT 10u
#define WS2812_DEFAULT_LED_COUNT 138u
#define WS2812_MAX_LEDS_PER_CHANNEL 192u

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812_rgb_t;

typedef enum {
    WS2812_WIRE_ORDER_GRB,
    WS2812_WIRE_ORDER_RGB,
    WS2812_WIRE_ORDER_BRG,
    WS2812_WIRE_ORDER_BGR,
    WS2812_WIRE_ORDER_RBG,
    WS2812_WIRE_ORDER_GBR
} ws2812_wire_order_t;

typedef struct {
    uint8_t gpio;
    uint16_t led_count;
    uint8_t brightness;
    ws2812_wire_order_t wire_order;
} ws2812_channel_config_t;

typedef enum {
    WS2812_STATUS_OK = 0,
    WS2812_STATUS_INVALID_ARGUMENT,
    WS2812_STATUS_OUT_OF_RANGE,
    WS2812_STATUS_BUSY
} ws2812_status_t;

void ws2812_init(void);
ws2812_status_t ws2812_configure_channel(uint8_t channel, const ws2812_channel_config_t *config);
ws2812_status_t ws2812_set_pixel(uint8_t channel, uint16_t index, ws2812_rgb_t color);
ws2812_status_t ws2812_get_pixel(uint8_t channel, uint16_t index, ws2812_rgb_t *color);
ws2812_status_t ws2812_clear_channel(uint8_t channel);
void ws2812_mark_dirty(uint8_t channel);
bool ws2812_channel_dirty(uint8_t channel);
void ws2812_service(void);

#endif
