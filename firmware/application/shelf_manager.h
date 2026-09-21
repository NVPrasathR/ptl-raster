#ifndef RASTER_SHELF_MANAGER_H
#define RASTER_SHELF_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "channel_manager.h"

#define SHELF_DEFAULT_SIZE 6u

typedef struct {
    uint16_t first_led;
    uint8_t led_count;
} shelf_range_t;

bool shelf_manager_set_size(uint8_t channel, uint8_t shelf_size);
bool shelf_manager_get_count(uint8_t channel, uint16_t *count);
bool shelf_manager_get_range(uint8_t channel, uint16_t shelf, shelf_range_t *range);
bool shelf_manager_set_color(uint8_t channel, uint16_t shelf, color_id_t color, uint16_t team_id, bool on);

#endif
