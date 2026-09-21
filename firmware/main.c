#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "board_config.h"
#include "board_pins.h"
#include "application/channel_manager.h"
#include "application/pharmacy_protocol.h"
#include "configuration/config_manager.h"
#include "drivers/button.h"
#include "drivers/buzzer.h"
#include "drivers/oled.h"
#include "drivers/oled_manager.h"
#include "drivers/status_led.h"
#include "network/w5500_eth.h"
#include "network/http_server.h"
#include "ota/ota_verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static raster_config_t device_config;
static w5500_eth_context_t ethernet;
static pharmacy_protocol_context_t protocol;

static bool authorize_admin(void *context, const char *credential) {
    const raster_config_t *config = (const raster_config_t *)context;
    static const char bearer[] = "Bearer ";
    uint8_t digest[32];
    uint8_t difference = 0u;
    if (config == NULL || !config->admin_credential_provisioned ||
        credential == NULL ||
        strncmp(credential, bearer, sizeof(bearer) - 1u) != 0 ||
        ota_digest_compute((const uint8_t *)credential + sizeof(bearer) - 1u,
                           strlen(credential) - (sizeof(bearer) - 1u),
                           digest) != 0) {
        return false;
    }
    for (size_t index = 0; index < sizeof(digest); ++index) {
        difference |= digest[index] ^ config->admin_credential_sha256[index];
    }
    return difference == 0u;
}

static void ipv4_to_string(raster_ipv4_t address, char output[16]) {
    snprintf(output, 16u, "%u.%u.%u.%u", address.octet[0], address.octet[1],
             address.octet[2], address.octet[3]);
}

static bool string_to_ipv4(const char *text, raster_ipv4_t *address) {
    unsigned int octets[4];
    char trailing;
    if (text == NULL || address == NULL ||
        sscanf(text, "%u.%u.%u.%u%c", &octets[0], &octets[1],
               &octets[2], &octets[3], &trailing) != 4) {
        return false;
    }
    for (size_t index = 0; index < 4u; ++index) {
        if (octets[index] > 255u) return false;
        address->octet[index] = (uint8_t)octets[index];
    }
    return true;
}

static bool commit_led_state(void *context, size_t channel,
                             const pharmacy_channel_state_t *state) {
    (void)context;
    if (state == NULL || channel >= RASTER_CHANNEL_COUNT ||
        !channel_manager_set_enabled((uint8_t)channel, state->enabled)) {
        return false;
    }
    for (uint16_t index = 0; index < state->led_count; ++index) {
        char *end = NULL;
        unsigned long team = state->leds[index].team_id[0] == '\0'
                                 ? 0u
                                 : strtoul(state->leds[index].team_id, &end, 10);
        if ((end != NULL && *end != '\0') || team > UINT16_MAX ||
            !channel_manager_set_led_rgb(
                (uint8_t)channel, index,
                (ws2812_rgb_t){state->leds[index].red, state->leds[index].green,
                               state->leds[index].blue},
                (uint16_t)team, state->leds[index].on)) {
            return false;
        }
    }
    return true;
}

static bool commit_network_config(void *context,
                                  const pharmacy_network_config_t *network) {
    raster_config_t previous = device_config;
    w5500_eth_context_t previous_ethernet = ethernet;
    raster_config_t next = device_config;
    (void)context;
    if (network == NULL ||
        !string_to_ipv4(network->ip_address, &next.network.address) ||
        !string_to_ipv4(network->subnet_mask, &next.network.netmask) ||
        !string_to_ipv4(network->gateway, &next.network.gateway) ||
        !string_to_ipv4(network->dns_server, &next.network.dns)) {
        return false;
    }
    next.network.dhcp_enabled = network->dhcp_enabled;
    next.network.http_port = (uint16_t)network->http_port;
    snprintf(next.network.device_name, sizeof(next.network.device_name), "%s",
             network->device_name);
    if (!raster_config_validate(&next) ||
        raster_config_save(&next) != RASTER_CONFIG_OK) {
        return false;
    }
    w5500_eth_set_static_config(&ethernet, network->ip_address,
                                network->subnet_mask, network->gateway,
                                network->dns_server, network->http_port,
                                network->dhcp_enabled);
    ethernet.state = W5500_NET_STATE_INIT;
    ethernet.socket_ready = false;
    if (!w5500_hw_init(&ethernet)) {
        (void)raster_config_save(&previous);
        ethernet = previous_ethernet;
        return false;
    }
    device_config = next;
    return true;
}

static bool commit_channel_config(void *context, size_t channel, uint16_t led_count,
                                  uint8_t brightness, uint8_t leds_per_shelf) {
    raster_config_t next = device_config;
    (void)context;
    if (channel >= RASTER_CHANNEL_COUNT) return false;
    next.channels[channel].led_count = led_count;
    next.channels[channel].brightness = brightness;
    next.channels[channel].leds_per_shelf = leds_per_shelf;
    if (!raster_config_validate(&next) ||
        !channel_manager_configure((uint8_t)channel, led_count, brightness,
                                   (ws2812_wire_order_t)next.ws2812_color_order)) {
        return false;
    }
    if (raster_config_save(&next) != RASTER_CONFIG_OK) {
        const raster_channel_config_t *previous = &device_config.channels[channel];
        (void)channel_manager_configure(
            (uint8_t)channel, previous->led_count, previous->brightness,
            (ws2812_wire_order_t)device_config.ws2812_color_order);
        return false;
    }
    device_config = next;
    return true;
}

static int handle_http_request(const char *raw, size_t raw_length, char *response,
                               size_t response_capacity, void *context) {
    http_request_t request;
    int status;
    (void)context;
    if (http_server_parse_request(raw, raw_length, &request) != 0) {
        int length = snprintf(response, response_capacity,
                              "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n"
                              "Content-Length: 0\r\n\r\n");
        return length > 0 ? length : 0;
    }
    if (http_server_route_request(&protocol, &request, response,
                                  response_capacity, &status) != 0) {
        return 0;
    }
    return (int)strlen(response);
}

static void initialize_peripherals(void) {
    const status_led_config_t status_config = {
        .gpio = RASTER_STATUS_LED_PIN,
        .polarity = STATUS_LED_ACTIVE_HIGH,
    };
    const button_config_t button_config = {
        .gpio = RASTER_USER_BUTTON_PIN,
        .polarity = BUTTON_ACTIVE_LOW,
        .type = BUTTON_TYPE_MOMENTARY,
        .debounce_ms = 30u,
        .long_press_ms = 3000u,
    };
    const buzzer_config_t buzzer_config = {
        .gpio = RASTER_BUZZER_PIN,
        .polarity = BUZZER_ACTIVE_HIGH,
    };

    status_led_init(&status_config);
    status_led_blink(500u, 500u);
    button_init(&button_config);
    buzzer_init(&buzzer_config);
    (void)channel_manager_init();
    (void)oled_manager_init();
}

static void apply_configuration(void) {
    char ip[16], subnet[16], gateway[16], dns[16];
    for (uint8_t channel = 0; channel < RASTER_CHANNEL_COUNT; ++channel) {
        (void)channel_manager_configure(
            channel, device_config.channels[channel].led_count,
            device_config.channels[channel].brightness,
            (ws2812_wire_order_t)device_config.ws2812_color_order);
        (void)channel_manager_set_enabled(channel, device_config.channels[channel].enabled);
    }
    ipv4_to_string(device_config.network.address, ip);
    ipv4_to_string(device_config.network.netmask, subnet);
    ipv4_to_string(device_config.network.gateway, gateway);
    ipv4_to_string(device_config.network.dns, dns);
    w5500_eth_init(&ethernet, device_config.network.mac,
                   device_config.network.dhcp_enabled, ip, subnet, gateway, dns,
                   device_config.network.device_name, device_config.network.http_port);
    pharmacy_protocol_init(&protocol);
    for (uint8_t channel = 0; channel < RASTER_CHANNEL_COUNT; ++channel) {
        protocol.channels[channel].led_count =
            device_config.channels[channel].led_count;
        protocol.channels[channel].enabled =
            device_config.channels[channel].enabled;
    }
    pharmacy_protocol_set_authorizer(&protocol, authorize_admin, &device_config);
    pharmacy_protocol_set_committers(&protocol, commit_led_state, NULL,
                                     commit_network_config, NULL,
                                     commit_channel_config, NULL);
}

int main(void) {
    stdio_init_all();
    initialize_peripherals();
    (void)raster_config_load(&device_config);
    apply_configuration();
    (void)w5500_hw_init(&ethernet);
    if (device_config.buzzer_enabled) {
        (void)buzzer_play(device_config.boot_beep_ms, 0u, 1u);
    }
    watchdog_enable(2000u, true);

    uint32_t last_tick_ms = 0u;
    bool previous_link = false;

    while (true) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        if ((now_ms - last_tick_ms) >= RASTER_TASK_TICK_MS) {
            last_tick_ms = now_ms;
            button_service(now_ms);
            if (button_take_long_press()) {
                status_led_blink(100u, 100u);
            } else if (button_take_press()) {
                status_led_blink(250u, 750u);
            }
            buzzer_service(now_ms);
            status_led_service(now_ms);
            oled_manager_service();
            channel_manager_service();
            bool link_up = w5500_hw_link_up();
            w5500_eth_update(&ethernet, now_ms, link_up);
            if (link_up != previous_link) {
                if (link_up) {
                    status_led_set(true);
                } else {
                    status_led_blink(150u, 1850u);
                }
                previous_link = link_up;
            }
            if (link_up) {
                w5500_http_service(&ethernet, handle_http_request, NULL);
            }
            watchdog_update();
        }

        tight_loop_contents();
    }

    return 0;
}
