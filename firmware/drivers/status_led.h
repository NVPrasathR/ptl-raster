#ifndef RASTER_STATUS_LED_H
#define RASTER_STATUS_LED_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STATUS_LED_ACTIVE_HIGH,
    STATUS_LED_ACTIVE_LOW
} status_led_polarity_t;

typedef struct {
    uint8_t gpio;
    status_led_polarity_t polarity;
} status_led_config_t;

void status_led_init(const status_led_config_t *config);
void status_led_set(bool on);
void status_led_blink(uint16_t on_ms, uint16_t off_ms);
void status_led_service(uint32_t now_ms);
bool status_led_is_on(void);

#endif
