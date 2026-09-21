#include "color_manager.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    const char *name;
    color_id_t id;
    ws2812_rgb_t rgb;
} color_entry_t;

static ws2812_rgb_t custom_color;
static const color_entry_t colors[] = {
    {"OFF", COLOR_OFF, {0u, 0u, 0u}}, {"WHITE", COLOR_WHITE, {255u, 255u, 255u}},
    {"VIOLET", COLOR_VIOLET, {255u, 0u, 255u}}, {"RED", COLOR_RED, {255u, 0u, 0u}},
    {"GREEN", COLOR_GREEN, {0u, 255u, 0u}}, {"BLUE", COLOR_BLUE, {0u, 0u, 255u}},
    {"YELLOW", COLOR_YELLOW, {255u, 255u, 0u}}
};

static bool equal_ignore_case(const char *left, const char *right) {
    if (left == NULL || right == NULL) return false;
    while (*left != '\0' && *right != '\0') {
        if (toupper((unsigned char)*left++) != toupper((unsigned char)*right++)) return false;
    }
    return *left == '\0' && *right == '\0';
}

bool color_manager_parse(const char *name, color_id_t *color) {
    if (color == NULL) return false;
    for (size_t index = 0; index < sizeof(colors) / sizeof(colors[0]); ++index) {
        if (equal_ignore_case(name, colors[index].name)) {
            *color = colors[index].id;
            return true;
        }
    }
    return false;
}

bool color_manager_get(color_id_t color, ws2812_rgb_t *rgb) {
    if (rgb == NULL) return false;
    if (color == COLOR_CUSTOM) {
        *rgb = custom_color;
        return true;
    }
    for (size_t index = 0; index < sizeof(colors) / sizeof(colors[0]); ++index) {
        if (colors[index].id == color) {
            *rgb = colors[index].rgb;
            return true;
        }
    }
    return false;
}

bool color_manager_set_custom(ws2812_rgb_t rgb) {
    custom_color = rgb;
    return true;
}

const char *color_manager_name(color_id_t color) {
    if (color == COLOR_CUSTOM) return "CUSTOM";
    for (size_t index = 0; index < sizeof(colors) / sizeof(colors[0]); ++index) {
        if (colors[index].id == color) return colors[index].name;
    }
    return NULL;
}
