#ifndef RASTER_BUZZER_H
#define RASTER_BUZZER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUZZER_ACTIVE_HIGH,
    BUZZER_ACTIVE_LOW
} buzzer_polarity_t;

typedef struct {
    uint8_t gpio;
    buzzer_polarity_t polarity;
} buzzer_config_t;

void buzzer_init(const buzzer_config_t *config);
bool buzzer_play(uint16_t on_ms, uint16_t off_ms, uint8_t repetitions);
void buzzer_stop(void);
void buzzer_service(uint32_t now_ms);
bool buzzer_is_active(void);

#endif
