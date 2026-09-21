#ifndef RASTER_CONFIG_MANAGER_H
#define RASTER_CONFIG_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RASTER_CONFIG_VERSION 1u
#define RASTER_CHANNEL_COUNT 10u
#define RASTER_DEFAULT_LED_COUNT 138u
#define RASTER_DEFAULT_LEDS_PER_SHELF 6u
#define RASTER_MAX_LEDS_PER_CHANNEL 192u
#define RASTER_DEVICE_NAME_MAX 32u
#define RASTER_CHANNEL_NAME_MAX 16u
#define RASTER_SHELF_LABEL_MAX 16u
#define RASTER_MAX_SHELF_LABELS 43u

typedef struct {
    uint8_t octet[4];
} raster_ipv4_t;

typedef struct {
    bool dhcp_enabled;
    raster_ipv4_t address;
    raster_ipv4_t netmask;
    raster_ipv4_t gateway;
    raster_ipv4_t dns;
    uint16_t http_port;
    char device_name[RASTER_DEVICE_NAME_MAX];
    uint8_t mac[6];
} raster_network_config_t;

typedef struct {
    uint16_t led_count;
    uint8_t leds_per_shelf;
    uint8_t brightness;
    bool enabled;
    char name[RASTER_CHANNEL_NAME_MAX];
    char shelf_labels[RASTER_MAX_SHELF_LABELS][RASTER_SHELF_LABEL_MAX];
} raster_channel_config_t;

typedef struct {
    uint32_t version;
    raster_network_config_t network;
    raster_channel_config_t channels[RASTER_CHANNEL_COUNT];
    bool buzzer_enabled;
    uint16_t boot_beep_ms;
    bool status_led_active_high;
    uint8_t ws2812_color_order;
    bool admin_credential_provisioned;
    uint8_t admin_credential_sha256[32];
    uint8_t reserved[12];
} raster_config_t;

typedef enum {
    RASTER_CONFIG_OK = 0,
    RASTER_CONFIG_DEFAULTED,
    RASTER_CONFIG_INVALID_ARGUMENT,
    RASTER_CONFIG_INVALID_VALUE,
    RASTER_CONFIG_STORAGE_ERROR
} raster_config_result_t;

void raster_config_defaults(raster_config_t *config);
bool raster_config_validate(const raster_config_t *config);
raster_config_result_t raster_config_load(raster_config_t *config);
raster_config_result_t raster_config_save(const raster_config_t *config);
raster_config_result_t raster_config_factory_reset(raster_config_t *config);
uint32_t raster_config_crc32(const void *data, size_t length);

#endif
