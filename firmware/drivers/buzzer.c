#include "buzzer.h"

#include <stddef.h>

#if defined(PICO_ON_DEVICE)
#include "hardware/gpio.h"
#endif

static buzzer_config_t buzzer_config;
static uint32_t deadline_ms;
static uint16_t on_duration_ms;
static uint16_t off_duration_ms;
static uint8_t remaining_repetitions;
static bool output_on;

static void set_output(bool on) {
    output_on = on;
#if defined(PICO_ON_DEVICE)
    gpio_put(buzzer_config.gpio, on == (buzzer_config.polarity == BUZZER_ACTIVE_HIGH));
#endif
}

void buzzer_init(const buzzer_config_t *config) {
    if (config == NULL) return;
    buzzer_config = *config;
    remaining_repetitions = 0u;
#if defined(PICO_ON_DEVICE)
    gpio_init(buzzer_config.gpio);
    gpio_set_dir(buzzer_config.gpio, GPIO_OUT);
#endif
    set_output(false);
}

bool buzzer_play(uint16_t on_ms, uint16_t off_ms, uint8_t repetitions) {
    if (on_ms == 0u || repetitions == 0u) return false;
    on_duration_ms = on_ms;
    off_duration_ms = off_ms;
    remaining_repetitions = repetitions;
    output_on = false;
    deadline_ms = 0u;
    return true;
}

void buzzer_stop(void) {
    remaining_repetitions = 0u;
    set_output(false);
}

void buzzer_service(uint32_t now_ms) {
    if (remaining_repetitions == 0u || (int32_t)(now_ms - deadline_ms) < 0) return;
    if (!output_on) {
        set_output(true);
        deadline_ms = now_ms + on_duration_ms;
    } else {
        set_output(false);
        --remaining_repetitions;
        deadline_ms = now_ms + off_duration_ms;
    }
}

bool buzzer_is_active(void) {
    return remaining_repetitions != 0u;
}
