#ifndef RASTER_CHANNEL_MANAGER_H
#define RASTER_CHANNEL_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "color_manager.h"

typedef struct {
    color_id_t color;
    ws2812_rgb_t rgb;
    uint16_t team_id;
    bool on;
} channel_led_state_t;

typedef struct {
    uint16_t led_count;
    uint8_t brightness;
    bool enabled;
} channel_info_t;

bool channel_manager_init(void);
bool channel_manager_configure(uint8_t channel, uint16_t led_count, uint8_t brightness,
                               ws2812_wire_order_t wire_order);
bool channel_manager_set_led(uint8_t channel, uint16_t led_index, color_id_t color,
                             uint16_t team_id, bool on);
bool channel_manager_set_led_rgb(uint8_t channel, uint16_t led_index, ws2812_rgb_t rgb,
                                 uint16_t team_id, bool on);
bool channel_manager_get_led(uint8_t channel, uint16_t led_index, channel_led_state_t *state);
bool channel_manager_get_info(uint8_t channel, channel_info_t *info);
bool channel_manager_set_enabled(uint8_t channel, bool enabled);
bool channel_manager_clear(uint8_t channel);
void channel_manager_clear_all(void);
void channel_manager_service(void);

#endif
