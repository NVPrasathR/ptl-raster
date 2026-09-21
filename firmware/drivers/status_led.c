#include "status_led.h"

#include <stddef.h>

#if defined(PICO_ON_DEVICE)
#include "hardware/gpio.h"
#endif

static status_led_config_t led_config;
static bool led_on;
static bool blinking;
static uint16_t blink_on_ms;
static uint16_t blink_off_ms;
static uint32_t blink_deadline_ms;

static void write_led(bool on) {
    led_on = on;
#if defined(PICO_ON_DEVICE)
    gpio_put(led_config.gpio, on == (led_config.polarity == STATUS_LED_ACTIVE_HIGH));
#endif
}

void status_led_init(const status_led_config_t *config) {
    if (config == NULL) return;
    led_config = *config;
#if defined(PICO_ON_DEVICE)
    gpio_init(led_config.gpio);
    gpio_set_dir(led_config.gpio, GPIO_OUT);
#endif
    status_led_set(false);
}

void status_led_set(bool on) {
    blinking = false;
    write_led(on);
}

void status_led_blink(uint16_t on_ms, uint16_t off_ms) {
    if (on_ms == 0u || off_ms == 0u) return;
    blinking = true;
    blink_on_ms = on_ms;
    blink_off_ms = off_ms;
    blink_deadline_ms = 0u;
}

void status_led_service(uint32_t now_ms) {
    if (!blinking || (int32_t)(now_ms - blink_deadline_ms) < 0) return;
    write_led(!led_on);
    blink_deadline_ms = now_ms + (led_on ? blink_on_ms : blink_off_ms);
}

bool status_led_is_on(void) {
    return led_on;
}
