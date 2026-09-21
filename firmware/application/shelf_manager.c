#include "shelf_manager.h"

static uint8_t shelf_sizes[WS2812_CHANNEL_COUNT] = {
    SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE,
    SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE, SHELF_DEFAULT_SIZE
};

bool shelf_manager_set_size(uint8_t channel, uint8_t shelf_size) {
    if (channel >= WS2812_CHANNEL_COUNT || shelf_size == 0u || shelf_size > WS2812_MAX_LEDS_PER_CHANNEL) return false;
    shelf_sizes[channel] = shelf_size;
    return true;
}

bool shelf_manager_get_count(uint8_t channel, uint16_t *count) {
    channel_info_t info;
    if (count == NULL || !channel_manager_get_info(channel, &info)) return false;
    *count = (uint16_t)((info.led_count + shelf_sizes[channel] - 1u) / shelf_sizes[channel]);
    return true;
}

bool shelf_manager_get_range(uint8_t channel, uint16_t shelf, shelf_range_t *range) {
    channel_info_t info;
    uint16_t first;
    if (range == NULL || !channel_manager_get_info(channel, &info)) return false;
    first = (uint16_t)(shelf * shelf_sizes[channel]);
    if (first >= info.led_count) return false;
    range->first_led = first;
    range->led_count = (uint8_t)((info.led_count - first) < shelf_sizes[channel] ?
                                     (info.led_count - first) : shelf_sizes[channel]);
    return true;
}

bool shelf_manager_set_color(uint8_t channel, uint16_t shelf, color_id_t color, uint16_t team_id, bool on) {
    shelf_range_t range;
    if (!shelf_manager_get_range(channel, shelf, &range)) return false;
    for (uint8_t offset = 0; offset < range.led_count; ++offset) {
        if (!channel_manager_set_led(channel, (uint16_t)(range.first_led + offset), color, team_id, on)) return false;
    }
    return true;
}
