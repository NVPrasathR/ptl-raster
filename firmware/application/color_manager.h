#ifndef RASTER_COLOR_MANAGER_H
#define RASTER_COLOR_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "../drivers/ws2812.h"

typedef enum {
    COLOR_OFF,
    COLOR_WHITE,
    COLOR_VIOLET,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CUSTOM
} color_id_t;

bool color_manager_parse(const char *name, color_id_t *color);
bool color_manager_get(color_id_t color, ws2812_rgb_t *rgb);
bool color_manager_set_custom(ws2812_rgb_t rgb);
const char *color_manager_name(color_id_t color);

#endif
