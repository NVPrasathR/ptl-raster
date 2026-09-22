#ifndef PHARMACY_PROTOCOL_H
#define PHARMACY_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PHARMACY_MAX_CHANNELS 10U
#define PHARMACY_MAX_LEDS_PER_CHANNEL 192U
#define PHARMACY_MAX_TEAM_ID 32U
#define PHARMACY_MAX_JSON_BODY 4096U
#define PHARMACY_JSON_RESPONSE_CAPACITY 12288U

typedef struct {
    uint16_t led_no;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    char team_id[PHARMACY_MAX_TEAM_ID];
    bool on;
} pharmacy_led_assignment_t;

typedef struct {
    char channel[4];
    bool enabled;
    uint16_t led_count;
    pharmacy_led_assignment_t leds[PHARMACY_MAX_LEDS_PER_CHANNEL];
    uint16_t used_leds;
} pharmacy_channel_state_t;

typedef struct {
    char company_name[32];
    char product_name[32];
    char firmware_version[16];
    char hardware_revision[16];
    char ip_address[16];
    char mac_address[18];
    uint32_t uptime_seconds;
    bool network_connected;
    bool ota_active;
} pharmacy_device_info_t;

typedef struct {
    bool dhcp_enabled;
    char ip_address[16];
    char subnet_mask[16];
    char gateway[16];
    char dns_server[16];
    int http_port;
    char device_name[32];
} pharmacy_network_config_t;

typedef struct {
    bool successful;
    char channel[4];
    char status[8];
    int updated_leds;
    char message[128];
    char error_code[32];
    char error_message[128];
} pharmacy_response_t;

typedef bool (*pharmacy_authorize_fn)(void *context, const char *credential);
typedef bool (*pharmacy_led_commit_fn)(void *context, size_t channel,
                                       const pharmacy_channel_state_t *state);
typedef bool (*pharmacy_config_commit_fn)(void *context,
                                          const pharmacy_network_config_t *config);
typedef bool (*pharmacy_channel_config_fn)(void *context, size_t channel,
                                           uint16_t led_count, uint8_t brightness,
                                           uint8_t leds_per_shelf);

typedef struct pharmacy_protocol_context_t {
    pharmacy_channel_state_t channels[PHARMACY_MAX_CHANNELS];
    pharmacy_network_config_t network_config;
    pharmacy_device_info_t device_info;
    bool ota_active;
    size_t channel_count;
    pharmacy_authorize_fn authorize;
    void *authorize_context;
    pharmacy_led_commit_fn led_commit;
    void *led_commit_context;
    pharmacy_config_commit_fn config_commit;
    void *config_commit_context;
    pharmacy_channel_config_fn channel_config;
    void *channel_config_context;
    bool request_authorized;
    bool allow_unprovisioned_led_control;
} pharmacy_protocol_context_t;

void pharmacy_protocol_init(pharmacy_protocol_context_t *ctx);
void pharmacy_protocol_set_authorizer(pharmacy_protocol_context_t *ctx,
                                      pharmacy_authorize_fn authorize,
                                      void *authorize_context);
void pharmacy_protocol_set_committers(pharmacy_protocol_context_t *ctx,
                                      pharmacy_led_commit_fn led_commit,
                                      void *led_commit_context,
                                      pharmacy_config_commit_fn config_commit,
                                      void *config_commit_context,
                                      pharmacy_channel_config_fn channel_config,
                                      void *channel_config_context);
void pharmacy_protocol_sync_network(pharmacy_protocol_context_t *ctx,
                                    const char *ip_address,
                                    bool network_connected,
                                    bool dhcp_enabled);

int pharmacy_protocol_handle_request(pharmacy_protocol_context_t *ctx,
                                    const char *method,
                                    const char *path,
                                    const char *body,
                                    size_t body_len,
                                    char *response,
                                    size_t response_cap);

int pharmacy_protocol_apply_led_control(pharmacy_protocol_context_t *ctx,
                                       const char *json_body,
                                       size_t body_len,
                                       pharmacy_response_t *response);

int pharmacy_protocol_apply_server_data(pharmacy_protocol_context_t *ctx,
                                        const char *json_body,
                                        size_t body_len);

bool pharmacy_protocol_parse_channel_name(const char *text, size_t *index_out);
bool pharmacy_protocol_is_valid_team_id(const char *team_id);

#ifdef __cplusplus
}
#endif

#endif
