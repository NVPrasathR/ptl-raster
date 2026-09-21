#ifndef RASTER_BUTTON_H
#define RASTER_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUTTON_ACTIVE_LOW,
    BUTTON_ACTIVE_HIGH
} button_polarity_t;

typedef enum {
    BUTTON_TYPE_MOMENTARY,
    BUTTON_TYPE_LATCHING
} button_type_t;

typedef struct {
    uint8_t gpio;
    button_polarity_t polarity;
    button_type_t type;
    uint16_t debounce_ms;
    uint16_t long_press_ms;
} button_config_t;

void button_init(const button_config_t *config);
void button_service(uint32_t now_ms);
bool button_take_press(void);
bool button_take_long_press(void);
bool button_is_pressed(void);

#endif
