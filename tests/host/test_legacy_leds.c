#include <stdio.h>
#include <string.h>

#include "pharmacy_protocol.h"

int main(void) {
    pharmacy_protocol_context_t ctx;
    pharmacy_response_t response;
    const char *legacy_json = "{\"channel\":\"C04\",\"status\":\"on\",\"leds\":\"1:RED|2:BLUE\"}";

    pharmacy_protocol_init(&ctx);
    if (pharmacy_protocol_apply_led_control(&ctx, legacy_json, strlen(legacy_json), &response) != 0) {
        fprintf(stderr,
                "legacy parser gap: expected legacy LEDs to apply, got %s: %s\n",
                response.error_code,
                response.error_message);
        return 1;
    }
    if (!ctx.channels[3].leds[0].on || ctx.channels[3].leds[0].red != 255u ||
        !ctx.channels[3].leds[1].on || ctx.channels[3].leds[1].blue != 255u) {
        fprintf(stderr, "legacy parser gap: legacy LEDs were not materialized correctly\n");
        return 1;
    }
    return 0;
}
