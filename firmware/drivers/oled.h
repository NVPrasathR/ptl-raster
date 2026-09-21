#ifndef RASTER_OLED_H
#define RASTER_OLED_H

#include <stdbool.h>
#include <stdint.h>

#ifndef OLED_ASSUME_SSD1306
#define OLED_ASSUME_SSD1306 0
#endif

#define OLED_MAX_WIDTH 128u
#define OLED_MAX_HEIGHT 64u

typedef struct {
    uint8_t i2c_instance;
    uint8_t sda_gpio;
    uint8_t scl_gpio;
    uint8_t i2c_address;
    uint8_t width;
    uint8_t height;
} oled_config_t;

typedef enum {
    OLED_STATUS_OK = 0,
    OLED_STATUS_INVALID_ARGUMENT,
    OLED_STATUS_UNSUPPORTED_CONTROLLER
} oled_status_t;

oled_status_t oled_init(const oled_config_t *config);
void oled_clear(void);
bool oled_set_pixel(uint8_t x, uint8_t y, bool on);
void oled_mark_dirty(void);
bool oled_is_dirty(void);
void oled_service(void);

#endif
