#include "pico/stdlib.h"
#include "board_config.h"
#include "board_pins.h"
#include "system/raster_runtime.h"

static bool status_led_state = false;
static uint32_t last_heartbeat_ms = 0u;

void raster_system_init(void) {
    gpio_init(RASTER_STATUS_LED_PIN);
    gpio_set_dir(RASTER_STATUS_LED_PIN, GPIO_OUT);
    gpio_put(RASTER_STATUS_LED_PIN, 0);
    status_led_state = false;
    last_heartbeat_ms = 0u;
}

void raster_system_set_status_led(bool enabled) {
    status_led_state = enabled;
    gpio_put(RASTER_STATUS_LED_PIN, enabled ? 1 : 0);
}

void raster_system_heartbeat(uint32_t tick_ms) {
    if ((tick_ms - last_heartbeat_ms) >= RASTER_HEARTBEAT_MS) {
        last_heartbeat_ms = tick_ms;
        raster_system_set_status_led(!status_led_state);
    }
}

void raster_system_tick(uint32_t tick_ms) {
    (void)tick_ms;
    raster_system_heartbeat(tick_ms);
}
