#include "channel_manager.h"

#include <string.h>

typedef struct {
    channel_led_state_t leds[WS2812_MAX_LEDS_PER_CHANNEL];
    channel_info_t info;
} channel_state_t;

static channel_state_t channels[WS2812_CHANNEL_COUNT];

static bool valid_led(uint8_t channel, uint16_t led_index) {
    return channel < WS2812_CHANNEL_COUNT && led_index < channels[channel].info.led_count;
}

bool channel_manager_init(void) {
    ws2812_init();
    for (uint8_t channel = 0; channel < WS2812_CHANNEL_COUNT; ++channel) {
        channels[channel].info.led_count = WS2812_DEFAULT_LED_COUNT;
        channels[channel].info.brightness = 32u;
        channels[channel].info.enabled = true;
        if (!channel_manager_clear(channel)) return false;
    }
    return true;
}

bool channel_manager_configure(uint8_t channel, uint16_t led_count, uint8_t brightness,
                               ws2812_wire_order_t wire_order) {
    ws2812_channel_config_t config;
    if (channel >= WS2812_CHANNEL_COUNT || led_count == 0u || led_count > WS2812_MAX_LEDS_PER_CHANNEL) {
        return false;
    }
    config.gpio = channel;
    config.led_count = led_count;
    config.brightness = brightness;
    config.wire_order = wire_order;
    if (ws2812_configure_channel(channel, &config) != WS2812_STATUS_OK) return false;
    channels[channel].info.led_count = led_count;
    channels[channel].info.brightness = brightness;
    memset(channels[channel].leds, 0, sizeof(channels[channel].leds));
    return true;
}

bool channel_manager_set_led(uint8_t channel, uint16_t led_index, color_id_t color,
                             uint16_t team_id, bool on) {
    ws2812_rgb_t rgb;
    if (!valid_led(channel, led_index) || !color_manager_get(color, &rgb)) return false;
    channels[channel].leds[led_index].color = color;
    channels[channel].leds[led_index].rgb = rgb;
    channels[channel].leds[led_index].team_id = team_id;
    channels[channel].leds[led_index].on = on;
    if (!on || !channels[channel].info.enabled) rgb = (ws2812_rgb_t){0u, 0u, 0u};
    return ws2812_set_pixel(channel, led_index, rgb) == WS2812_STATUS_OK;
}

bool channel_manager_set_led_rgb(uint8_t channel, uint16_t led_index, ws2812_rgb_t rgb,
                                 uint16_t team_id, bool on) {
    if (!valid_led(channel, led_index)) return false;
    channels[channel].leds[led_index].color = COLOR_CUSTOM;
    channels[channel].leds[led_index].rgb = rgb;
    channels[channel].leds[led_index].team_id = team_id;
    channels[channel].leds[led_index].on = on;
    if (!on || !channels[channel].info.enabled) rgb = (ws2812_rgb_t){0u, 0u, 0u};
    return ws2812_set_pixel(channel, led_index, rgb) == WS2812_STATUS_OK;
}

bool channel_manager_get_led(uint8_t channel, uint16_t led_index, channel_led_state_t *state) {
    if (!valid_led(channel, led_index) || state == NULL) return false;
    *state = channels[channel].leds[led_index];
    return true;
}

bool channel_manager_get_info(uint8_t channel, channel_info_t *info) {
    if (channel >= WS2812_CHANNEL_COUNT || info == NULL) return false;
    *info = channels[channel].info;
    return true;
}

bool channel_manager_set_enabled(uint8_t channel, bool enabled) {
    if (channel >= WS2812_CHANNEL_COUNT) return false;
    channels[channel].info.enabled = enabled;
    for (uint16_t index = 0; index < channels[channel].info.led_count; ++index) {
        ws2812_rgb_t rgb = channels[channel].leds[index].rgb;
        if (!enabled || !channels[channel].leds[index].on) rgb = (ws2812_rgb_t){0u, 0u, 0u};
        if (ws2812_set_pixel(channel, index, rgb) != WS2812_STATUS_OK) return false;
    }
    return true;
}

bool channel_manager_clear(uint8_t channel) {
    if (channel >= WS2812_CHANNEL_COUNT) return false;
    memset(channels[channel].leds, 0, sizeof(channels[channel].leds));
    return ws2812_clear_channel(channel) == WS2812_STATUS_OK;
}

void channel_manager_clear_all(void) {
    for (uint8_t channel = 0; channel < WS2812_CHANNEL_COUNT; ++channel) (void)channel_manager_clear(channel);
}

void channel_manager_service(void) {
    ws2812_service();
}
