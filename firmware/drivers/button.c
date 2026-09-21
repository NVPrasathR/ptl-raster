#include "button.h"

#include <stddef.h>

#if defined(PICO_ON_DEVICE)
#include "hardware/gpio.h"
#endif

static button_config_t button_config;
static bool stable_pressed;
static bool sampled_pressed;
static bool press_pending;
static bool long_press_pending;
static uint32_t sample_changed_ms;
static uint32_t pressed_since_ms;

static bool read_pressed(void) {
#if defined(PICO_ON_DEVICE)
    bool level = gpio_get(button_config.gpio);
    return level == (button_config.polarity == BUTTON_ACTIVE_HIGH);
#else
    return false;
#endif
}

void button_init(const button_config_t *config) {
    if (config == NULL) return;
    button_config = *config;
#if defined(PICO_ON_DEVICE)
    gpio_init(button_config.gpio);
    gpio_set_dir(button_config.gpio, GPIO_IN);
    gpio_pull_up(button_config.gpio);
#endif
    stable_pressed = sampled_pressed = read_pressed();
    press_pending = false;
    long_press_pending = false;
    sample_changed_ms = 0u;
}

void button_service(uint32_t now_ms) {
    bool sample = read_pressed();
    if (sample != sampled_pressed) {
        sampled_pressed = sample;
        sample_changed_ms = now_ms;
    }
    if (sampled_pressed != stable_pressed &&
        (uint32_t)(now_ms - sample_changed_ms) >= button_config.debounce_ms) {
        stable_pressed = sampled_pressed;
        if (stable_pressed) {
            pressed_since_ms = now_ms;
            if (button_config.type == BUTTON_TYPE_LATCHING) press_pending = true;
        } else if (button_config.type == BUTTON_TYPE_MOMENTARY) {
            uint32_t duration = now_ms - pressed_since_ms;
            if (button_config.long_press_ms != 0u &&
                duration >= button_config.long_press_ms) {
                long_press_pending = true;
            } else {
                press_pending = true;
            }
        }
    }
}

bool button_take_long_press(void) {
    bool pressed = long_press_pending;
    long_press_pending = false;
    return pressed;
}

bool button_take_press(void) {
    bool pressed = press_pending;
    press_pending = false;
    return pressed;
}

bool button_is_pressed(void) {
    return stable_pressed;
}
