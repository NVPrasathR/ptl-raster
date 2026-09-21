#pragma once

#include <stdint.h>

#include "ptl_result.h"

typedef struct {
    uint8_t i2c_instance;
    uint8_t i2c_address;
    uint8_t sda_gpio;
    uint8_t scl_gpio;
    uint32_t i2c_frequency_hz;
    uint8_t width;
    uint8_t height;
    uint8_t column_offset;
    uint8_t orientation;
} sh1106g_config_t;

ptl_result_t sh1106g_init(const sh1106g_config_t *config);
void sh1106g_deinit(void);
void sh1106g_clear(void);
void sh1106g_draw_text(uint8_t x, uint8_t y, const char *text);
void sh1106g_update(void);
