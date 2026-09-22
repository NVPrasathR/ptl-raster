#include "pharmacy_protocol.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_color_names[] = {
    "WHITE",
    "VIOLET",
    "RED",
    "GREEN",
    "BLUE",
    "YELLOW",
    "OFF"
};

static bool pharmacy_apply_assignment_set(pharmacy_protocol_context_t *ctx,
                                         const char *channel_name,
                                         const pharmacy_led_assignment_t *assignments,
                                         size_t assignment_count,
                                         const char *status_text,
                                         pharmacy_response_t *response);

static void pharmacy_response_reset(pharmacy_response_t *response) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
}

static bool pharmacy_parse_string_json_field(const char *json,
                                            const char *key,
                                            char *out_value,
                                            size_t out_cap) {
    char pattern[64];
    const char *match = NULL;
    const char *cursor = NULL;
    size_t pos = 0U;

    if (json == NULL || key == NULL || out_value == NULL || out_cap == 0U) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    match = strstr(json, pattern);
    if (match == NULL) {
        return false;
    }

    cursor = strchr(match, ':');
    if (cursor == NULL) {
        return false;
    }
    ++cursor;
    while (*cursor != '\0' && (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')) {
        ++cursor;
    }
    if (*cursor != '"') {
        return false;
    }
    ++cursor;
    while (*cursor != '\0' && *cursor != '"' && pos + 1U < out_cap) {
        out_value[pos++] = *cursor;
        ++cursor;
    }
    out_value[pos] = '\0';
    return pos > 0U;
}

static bool pharmacy_parse_int_json_field(const char *json, const char *key, int *out_value) {
    char pattern[64];
    const char *match = NULL;
    char *end = NULL;
    long parsed = 0;

    if (json == NULL || key == NULL || out_value == NULL) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    match = strstr(json, pattern);
    if (match == NULL) {
        return false;
    }

    match = strchr(match, ':');
    if (match == NULL) {
        return false;
    }
    ++match;
    while (*match != '\0' && (*match == ' ' || *match == '\t' || *match == '\n' || *match == '\r')) {
        ++match;
    }

    parsed = strtol(match, &end, 10);
    if (end == match || parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }
    *out_value = (int)parsed;
    return true;
}

static void pharmacy_color_to_rgb(const char *color_name, uint8_t *r, uint8_t *g, uint8_t *b) {
    size_t idx = 0U;
    unsigned int value = 0U;
    char hexbuf[7];

    if (r == NULL || g == NULL || b == NULL) {
        return;
    }
    *r = 0U; *g = 0U; *b = 0U;
    if (color_name == NULL) {
        return;
    }

    for (idx = 0U; idx < sizeof(k_color_names) / sizeof(k_color_names[0]); ++idx) {
        if (strcasecmp(color_name, k_color_names[idx]) == 0) {
            switch (idx) {
                case 0: *r = 255U; *g = 255U; *b = 255U; break;
                case 1: *r = 255U; *g = 0U; *b = 255U; break;
                case 2: *r = 255U; *g = 0U; *b = 0U; break;
                case 3: *r = 0U; *g = 255U; *b = 0U; break;
                case 4: *r = 0U; *g = 0U; *b = 255U; break;
                case 5: *r = 255U; *g = 255U; *b = 0U; break;
                default: break;
            }
            return;
        }
    }

    if (color_name[0] == '#') {
        size_t len = strlen(color_name + 1U);
        if (len == 6U) {
            memcpy(hexbuf, color_name + 1U, 6U);
            hexbuf[6] = '\0';
            if (sscanf(hexbuf, "%x", &value) == 1) {
                *r = (uint8_t)((value >> 16U) & 0xFFU);
                *g = (uint8_t)((value >> 8U) & 0xFFU);
                *b = (uint8_t)(value & 0xFFU);
            }
        }
    }
}

static bool pharmacy_color_name_is_valid(const char *color_name) {
    uint8_t r = 0U, g = 0U, b = 0U;
    size_t i = 0U;

    if (color_name == NULL || color_name[0] == '\0') {
        return false;
    }

    for (i = 0U; i < sizeof(k_color_names) / sizeof(k_color_names[0]); ++i) {
        if (strcasecmp(color_name, k_color_names[i]) == 0) {
            return true;
        }
    }

    if (color_name[0] == '#') {
        size_t len = strlen(color_name + 1U);
        if (len == 6U) {
            unsigned int value = 0U;
            char hexbuf[7];
            memcpy(hexbuf, color_name + 1U, 6U);
            hexbuf[6] = '\0';
            if (sscanf(hexbuf, "%x", &value) == 1) {
                r = (uint8_t)((value >> 16U) & 0xFFU);
                g = (uint8_t)((value >> 8U) & 0xFFU);
                b = (uint8_t)(value & 0xFFU);
                return true;
            }
        }
    }

    pharmacy_color_to_rgb(color_name, &r, &g, &b);
    return (r != 0U || g != 0U || b != 0U) || (strcasecmp(color_name, "OFF") == 0);
}

static bool pharmacy_channel_name_to_index(const char *channel_name, size_t *index_out) {
    unsigned int value = 0U;
    if (channel_name == NULL || index_out == NULL) {
        return false;
    }
    if (strlen(channel_name) != 3U || channel_name[0] != 'C') {
        return false;
    }
    if (!isdigit((unsigned char)channel_name[1]) || !isdigit((unsigned char)channel_name[2])) {
        return false;
    }
    value = (unsigned int)(channel_name[1] - '0') * 10U + (unsigned int)(channel_name[2] - '0');
    if (value < 1U || value > PHARMACY_MAX_CHANNELS) {
        return false;
    }
    *index_out = (size_t)(value - 1U);
    return true;
}

bool pharmacy_protocol_parse_channel_name(const char *text, size_t *index_out) {
    return pharmacy_channel_name_to_index(text, index_out);
}

bool pharmacy_protocol_is_valid_team_id(const char *team_id) {
    size_t idx = 0U;
    if (team_id == NULL || team_id[0] == '\0') {
        return false;
    }
    for (idx = 0U; team_id[idx] != '\0'; ++idx) {
        if (!isdigit((unsigned char)team_id[idx])) {
            return false;
        }
    }
    return true;
}

static bool pharmacy_parse_led_list_items(const char *json,
                                         pharmacy_led_assignment_t *assignments,
                                         size_t assignments_cap,
                                         size_t *assignment_count) {
    const char *array_start = NULL;
    const char *cursor = NULL;
    size_t count = 0U;

    if (json == NULL || assignments == NULL || assignment_count == NULL) {
        return false;
    }

    array_start = strstr(json, "\"led_list\"");
    if (array_start == NULL) {
        return false;
    }
    cursor = strchr(array_start, '[');
    if (cursor == NULL) {
        return false;
    }

    while (*cursor != '\0' && count < assignments_cap) {
        const char *object_start = strchr(cursor, '{');
        const char *object_end = NULL;
        char ledcolor[32];
        char team_id[PHARMACY_MAX_TEAM_ID];
        int led_number = 0;
        if (object_start == NULL) {
            break;
        }
        object_end = strchr(object_start, '}');
        if (object_end == NULL) {
            return false;
        }

        if (!pharmacy_parse_int_json_field(object_start, "led_no", &led_number)) {
            return false;
        }
        if (led_number < 1) {
            return false;
        }
        if (!pharmacy_parse_string_json_field(object_start, "ledcolor", ledcolor, sizeof(ledcolor))) {
            return false;
        }
        if (!pharmacy_parse_string_json_field(object_start, "team_id", team_id, sizeof(team_id))) {
            return false;
        }
        if (!pharmacy_color_name_is_valid(ledcolor)) {
            return false;
        }
        if (!pharmacy_protocol_is_valid_team_id(team_id)) {
            return false;
        }

        assignments[count].led_no = (uint16_t)led_number;
        assignments[count].on = true;
        snprintf(assignments[count].team_id, sizeof(assignments[count].team_id), "%s", team_id);
        pharmacy_color_to_rgb(ledcolor, &assignments[count].red, &assignments[count].green, &assignments[count].blue);
        ++count;
        cursor = object_end + 1U;
        while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t' || *cursor == ',') {
            ++cursor;
        }
    }

    *assignment_count = count;
    return count > 0U;
}

static bool pharmacy_parse_legacy_leds_string(const char *value,
                                             pharmacy_led_assignment_t *assignments,
                                             size_t assignments_cap,
                                             size_t *assignment_count) {
    const char *cursor = value;
    size_t count = 0U;

    if (value == NULL || assignments == NULL || assignment_count == NULL) {
        return false;
    }

    while (*cursor != '\0' && count < assignments_cap) {
        char token[64];
        char token_copy[64];
        size_t pos = 0U;
        char *colon = NULL;
        char color[32];
        unsigned long led_no = 0U;

        while (*cursor != '\0' && *cursor != '|') {
            if (pos + 1U >= sizeof(token)) {
                return false;
            }
            token[pos++] = *cursor;
            ++cursor;
        }
        token[pos] = '\0';
        if (*cursor == '|') {
            ++cursor;
        }
        if (token[0] == '\0') {
            continue;
        }

        snprintf(token_copy, sizeof(token_copy), "%s", token);
        colon = strchr(token_copy, ':');
        if (colon == NULL) {
            return false;
        }
        *colon = '\0';
        led_no = strtoul(token_copy, NULL, 10);
        if (led_no == 0U) {
            return false;
        }
        snprintf(color, sizeof(color), "%s", colon + 1U);
        if (!pharmacy_color_name_is_valid(color)) {
            return false;
        }
        assignments[count].led_no = (uint16_t)led_no;
        assignments[count].on = true;
        snprintf(assignments[count].team_id, sizeof(assignments[count].team_id), "0");
        pharmacy_color_to_rgb(color, &assignments[count].red, &assignments[count].green, &assignments[count].blue);
        ++count;
    }

    *assignment_count = count;
    return count > 0U;
}

static const char *pharmacy_find_json_object_end(const char *object_start) {
    size_t depth = 0U;
    bool in_string = false;
    bool escaped = false;

    if (object_start == NULL || *object_start != '{') {
        return NULL;
    }
    for (const char *cursor = object_start; *cursor != '\0'; ++cursor) {
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (*cursor == '\\') {
                escaped = true;
            } else if (*cursor == '"') {
                in_string = false;
            }
            continue;
        }
        if (*cursor == '"') {
            in_string = true;
        } else if (*cursor == '{') {
            ++depth;
        } else if (*cursor == '}' && depth-- == 1U) {
            return cursor;
        }
    }
    return NULL;
}

int pharmacy_protocol_apply_server_data(pharmacy_protocol_context_t *ctx,
                                        const char *json_body,
                                        size_t body_len) {
    static char object[PHARMACY_MAX_JSON_BODY];
    static char channel_name[4];
    static char status[8];
    static pharmacy_led_assignment_t assignments[PHARMACY_MAX_LEDS_PER_CHANNEL];
    static pharmacy_response_t response;
    const char *cursor = NULL;
    size_t applied = 0U;

    if (ctx == NULL || json_body == NULL || body_len == 0U ||
        body_len >= PHARMACY_MAX_JSON_BODY || json_body[0] != '[') {
        return -1;
    }

    cursor = json_body;
    while ((cursor = strstr(cursor, "\"channel\"")) != NULL) {
        const char *object_start = cursor;
        const char *object_end = NULL;
        size_t object_length = 0U;
        size_t assignment_count = 0U;

        while (object_start > json_body && object_start[-1] != '{') {
            --object_start;
        }
        if (*object_start != '{' ||
            (object_end = pharmacy_find_json_object_end(object_start)) == NULL) {
            return -1;
        }
        object_length = (size_t)(object_end - object_start) + 1U;
        if (object_length >= sizeof(object)) {
            return -1;
        }
        memcpy(object, object_start, object_length);
        object[object_length] = '\0';

        if (!pharmacy_parse_string_json_field(object, "channel", channel_name,
                                              sizeof(channel_name)) ||
            !pharmacy_parse_string_json_field(object, "status", status,
                                              sizeof(status))) {
            return -1;
        }
        if (strcmp(status, "on") == 0 &&
            !pharmacy_parse_led_list_items(object, assignments,
                                           PHARMACY_MAX_LEDS_PER_CHANNEL,
                                           &assignment_count)) {
            return -1;
        }
        pharmacy_response_reset(&response);
        if (!pharmacy_apply_assignment_set(ctx, channel_name, assignments,
                                           assignment_count, status, &response)) {
            return -1;
        }
        ++applied;
        cursor = object_end + 1U;
    }

    return applied > 0U ? 0 : -1;
}

static bool pharmacy_extract_legacy_leds(const char *json,
                                        pharmacy_led_assignment_t *assignments,
                                        size_t assignments_cap,
                                        size_t *assignment_count) {
    char value[512];
    if (json == NULL || assignments == NULL || assignment_count == NULL) {
        return false;
    }
    if (!pharmacy_parse_string_json_field(json, "leds", value, sizeof(value))) {
        return false;
    }
    return pharmacy_parse_legacy_leds_string(value, assignments, assignments_cap, assignment_count);
}

static void pharmacy_assign_default_channel_layout(pharmacy_protocol_context_t *ctx) {
    size_t channel_index = 0U;
    size_t led_index = 0U;

    if (ctx == NULL) {
        return;
    }
    for (channel_index = 0U; channel_index < PHARMACY_MAX_CHANNELS; ++channel_index) {
        snprintf(ctx->channels[channel_index].channel, sizeof(ctx->channels[channel_index].channel), "C%02zu", channel_index + 1U);
        ctx->channels[channel_index].enabled = false;
        ctx->channels[channel_index].led_count = 138U;
        ctx->channels[channel_index].used_leds = 0U;
        for (led_index = 0U; led_index < PHARMACY_MAX_LEDS_PER_CHANNEL; ++led_index) {
            ctx->channels[channel_index].leds[led_index].led_no = (uint16_t)(led_index + 1U);
            ctx->channels[channel_index].leds[led_index].red = 0U;
            ctx->channels[channel_index].leds[led_index].green = 0U;
            ctx->channels[channel_index].leds[led_index].blue = 0U;
            ctx->channels[channel_index].leds[led_index].team_id[0] = '\0';
            ctx->channels[channel_index].leds[led_index].on = false;
        }
    }
    ctx->channel_count = PHARMACY_MAX_CHANNELS;
}

static void pharmacy_build_device_info(pharmacy_protocol_context_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    snprintf(ctx->device_info.company_name, sizeof(ctx->device_info.company_name), "%s", "Raster");
    snprintf(ctx->device_info.product_name, sizeof(ctx->device_info.product_name), "%s", "Pick to Light");
    snprintf(ctx->device_info.firmware_version, sizeof(ctx->device_info.firmware_version), "%s", "v1.0.0");
    snprintf(ctx->device_info.hardware_revision, sizeof(ctx->device_info.hardware_revision), "%s", "RP2354B");
    snprintf(ctx->device_info.ip_address, sizeof(ctx->device_info.ip_address), "%s", ctx->network_config.ip_address);
    snprintf(ctx->device_info.mac_address, sizeof(ctx->device_info.mac_address), "%s", "02:00:00:00:00:01");
    ctx->device_info.network_connected = true;
    ctx->device_info.ota_active = ctx->ota_active;
    ctx->device_info.uptime_seconds += 1U;
}

void pharmacy_protocol_sync_network(pharmacy_protocol_context_t *ctx,
                                    const char *ip_address,
                                    bool network_connected,
                                    bool dhcp_enabled) {
    if (ctx == NULL) {
        return;
    }
    if (ip_address != NULL && ip_address[0] != '\0') {
        snprintf(ctx->network_config.ip_address, sizeof(ctx->network_config.ip_address), "%s", ip_address);
        snprintf(ctx->device_info.ip_address, sizeof(ctx->device_info.ip_address), "%s", ip_address);
    }
    ctx->network_config.dhcp_enabled = dhcp_enabled;
    ctx->device_info.network_connected = network_connected;
    ctx->device_info.ota_active = ctx->ota_active;
}

void pharmacy_protocol_init(pharmacy_protocol_context_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    pharmacy_assign_default_channel_layout(ctx);
    ctx->network_config.dhcp_enabled = true;
    ctx->network_config.http_port = 80;
    snprintf(ctx->network_config.ip_address, sizeof(ctx->network_config.ip_address), "%s", "172.17.0.102");
    snprintf(ctx->network_config.subnet_mask, sizeof(ctx->network_config.subnet_mask), "%s", "255.255.252.0");
    snprintf(ctx->network_config.gateway, sizeof(ctx->network_config.gateway), "%s", "172.17.3.254");
    snprintf(ctx->network_config.dns_server, sizeof(ctx->network_config.dns_server), "%s", "172.17.3.254");
    snprintf(ctx->network_config.device_name, sizeof(ctx->network_config.device_name), "%s", "raster-pick-to-light");
    ctx->ota_active = false;
    pharmacy_build_device_info(ctx);
}

void pharmacy_protocol_set_authorizer(pharmacy_protocol_context_t *ctx,
                                      pharmacy_authorize_fn authorize,
                                      void *authorize_context) {
    if (ctx == NULL) {
        return;
    }
    ctx->authorize = authorize;
    ctx->authorize_context = authorize_context;
    ctx->request_authorized = false;
}

void pharmacy_protocol_set_committers(pharmacy_protocol_context_t *ctx,
                                      pharmacy_led_commit_fn led_commit,
                                      void *led_commit_context,
                                      pharmacy_config_commit_fn config_commit,
                                      void *config_commit_context,
                                      pharmacy_channel_config_fn channel_config,
                                      void *channel_config_context) {
    if (ctx == NULL) {
        return;
    }
    ctx->led_commit = led_commit;
    ctx->led_commit_context = led_commit_context;
    ctx->config_commit = config_commit;
    ctx->config_commit_context = config_commit_context;
    ctx->channel_config = channel_config;
    ctx->channel_config_context = channel_config_context;
}

static bool pharmacy_apply_assignment_set(pharmacy_protocol_context_t *ctx,
                                         const char *channel_name,
                                         const pharmacy_led_assignment_t *assignments,
                                         size_t assignment_count,
                                         const char *status_text,
                                         pharmacy_response_t *response) {
    size_t channel_index = 0U;
    size_t idx = 0U;
    size_t seen[PHARMACY_MAX_LEDS_PER_CHANNEL] = { 0U };
    bool status_on = false;

    if (ctx == NULL || assignments == NULL || response == NULL) {
        return false;
    }
    if (!pharmacy_channel_name_to_index(channel_name, &channel_index)) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_CHANNEL");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "Invalid channel identifier");
        return false;
    }
    if (ctx->channels[channel_index].led_count == 0U) {
        ctx->channels[channel_index].led_count = 138U;
    }
    if (status_text == NULL) {
        status_on = true;
    } else if (strcmp(status_text, "on") == 0) {
        status_on = true;
    } else if (strcmp(status_text, "off") == 0) {
        status_on = false;
    } else {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_STATUS");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "status must be on or off");
        return false;
    }

    for (idx = 0U; idx < assignment_count; ++idx) {
        size_t led_index = (size_t)assignments[idx].led_no - 1U;
        if (assignments[idx].led_no == 0U || assignments[idx].led_no > ctx->channels[channel_index].led_count) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_LED_NUMBER");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "LED number exceeds channel length");
            return false;
        }
        if (seen[led_index] != 0U) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "DUPLICATE_LED");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "duplicate LED assignment detected");
            return false;
        }
        if (!pharmacy_protocol_is_valid_team_id(assignments[idx].team_id)) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_TEAM_ID");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "team_id must be numeric");
            return false;
        }
        seen[led_index] = 1U;
    }

    if (!status_on) {
        pharmacy_channel_state_t next = ctx->channels[channel_index];
        next.enabled = false;
        for (idx = 0U; idx < next.led_count; ++idx) {
            next.leds[idx].on = false;
            next.leds[idx].team_id[0] = '\0';
        }
        if (ctx->led_commit != NULL &&
            !ctx->led_commit(ctx->led_commit_context, channel_index, &next)) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "HARDWARE_COMMIT_FAILED");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "LED hardware rejected command");
            return false;
        }
        ctx->channels[channel_index] = next;
        snprintf(response->channel, sizeof(response->channel), "%s", channel_name);
        snprintf(response->status, sizeof(response->status), "%s", "off");
        response->successful = true;
        response->updated_leds = (int)ctx->channels[channel_index].led_count;
        snprintf(response->message, sizeof(response->message), "%s", "Channel disabled");
        return true;
    }

    pharmacy_channel_state_t next = ctx->channels[channel_index];
    for (idx = 0U; idx < assignment_count; ++idx) {
        size_t led_index = (size_t)assignments[idx].led_no - 1U;
        next.enabled = true;
        next.leds[led_index].led_no = assignments[idx].led_no;
        next.leds[led_index].red = assignments[idx].red;
        next.leds[led_index].green = assignments[idx].green;
        next.leds[led_index].blue = assignments[idx].blue;
        next.leds[led_index].on = true;
        snprintf(next.leds[led_index].team_id,
                 sizeof(next.leds[led_index].team_id),
                 "%s",
                 assignments[idx].team_id);
    }
    if (ctx->led_commit != NULL &&
        !ctx->led_commit(ctx->led_commit_context, channel_index, &next)) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "HARDWARE_COMMIT_FAILED");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "LED hardware rejected command");
        return false;
    }
    ctx->channels[channel_index] = next;
    snprintf(response->channel, sizeof(response->channel), "%s", channel_name);
    snprintf(response->status, sizeof(response->status), "%s", "on");
    response->successful = true;
    response->updated_leds = (int)assignment_count;
    snprintf(response->message, sizeof(response->message), "%s", "LED command applied");
    return true;
}

static size_t pharmacy_count_shelves(const char *json) {
    static const char shelf_key[] = "\"shelf_phr_id\"";
    const char *cursor = json;
    size_t count = 0U;
    while (cursor != NULL && (cursor = strstr(cursor, shelf_key)) != NULL) {
        ++count;
        cursor += sizeof(shelf_key) - 1U;
    }
    return count;
}

static const char *pharmacy_rgb_to_color_name(uint8_t red, uint8_t green, uint8_t blue) {
    if (red == 255U && green == 255U && blue == 255U) return "WHITE";
    if (red == 255U && green == 0U && blue == 255U) return "VIOLET";
    if (red == 255U && green == 0U && blue == 0U) return "RED";
    if (red == 0U && green == 255U && blue == 0U) return "GREEN";
    if (red == 0U && green == 0U && blue == 255U) return "BLUE";
    if (red == 255U && green == 255U && blue == 0U) return "YELLOW";
    return "OFF";
}

static int pharmacy_apply_picklight_control(pharmacy_protocol_context_t *ctx,
                                            const char *json_body,
                                            pharmacy_response_t *response) {
    pharmacy_led_assignment_t assignments[PHARMACY_MAX_LEDS_PER_CHANNEL];
    char team_color[32];
    char team_id[PHARMACY_MAX_TEAM_ID];
    char status[8];
    int location_id = 0;
    size_t channel_index = 0U;
    size_t shelf_count = 0U;
    size_t assignment_count = 0U;

    if (ctx == NULL || json_body == NULL || response == NULL ||
        !pharmacy_parse_string_json_field(json_body, "teamcolor", team_color, sizeof(team_color)) ||
        !pharmacy_parse_string_json_field(json_body, "team_id", team_id, sizeof(team_id)) ||
        !pharmacy_parse_string_json_field(json_body, "status", status, sizeof(status)) ||
        !pharmacy_parse_int_json_field(json_body, "location_id", &location_id) ||
        !pharmacy_color_name_is_valid(team_color) || !pharmacy_protocol_is_valid_team_id(team_id) ||
        (strcmp(status, "on") != 0 && strcmp(status, "off") != 0) ||
        location_id < 1 || (size_t)location_id > ctx->channel_count) {
        return -1;
    }

    channel_index = (size_t)(location_id - 1);
    shelf_count = pharmacy_count_shelves(json_body);
    if (shelf_count == 0U || shelf_count > (ctx->channels[channel_index].led_count + 5U) / 6U) {
        return -1;
    }

    for (size_t shelf_index = 0U; shelf_index < shelf_count; ++shelf_index) {
        for (size_t offset = 0U; offset < 6U; ++offset) {
            size_t led_index = shelf_index * 6U + offset;
            if (led_index >= ctx->channels[channel_index].led_count) break;
            assignments[assignment_count].led_no = (uint16_t)(led_index + 1U);
            assignments[assignment_count].on = strcmp(status, "on") == 0;
            snprintf(assignments[assignment_count].team_id,
                     sizeof(assignments[assignment_count].team_id), "%s", team_id);
            pharmacy_color_to_rgb(team_color, &assignments[assignment_count].red,
                                  &assignments[assignment_count].green,
                                  &assignments[assignment_count].blue);
            ++assignment_count;
        }
    }

    return pharmacy_apply_assignment_set(ctx, ctx->channels[channel_index].channel, assignments,
                                         assignment_count, status, response) ? 0 : -1;
}

static bool pharmacy_render_picklight_channel(const pharmacy_protocol_context_t *ctx,
                                              size_t channel_index,
                                              char *response,
                                              size_t response_cap) {
    const pharmacy_channel_state_t *channel = NULL;
    size_t used = 0U;
    int written = 0;

    if (ctx == NULL || response == NULL || channel_index >= ctx->channel_count) return false;
    channel = &ctx->channels[channel_index];
    written = snprintf(response, response_cap, "[{\"channel\":\"%s\",\"status\":\"%s\",\"leds\":\"",
                       channel->channel, channel->enabled ? "on" : "off");
    if (written < 0 || (size_t)written >= response_cap) return false;
    used = (size_t)written;
    for (size_t led_index = 0U; led_index < channel->led_count; ++led_index) {
        const pharmacy_led_assignment_t *led = &channel->leds[led_index];
        written = snprintf(response + used, response_cap - used, "%zu:%s|", led_index + 1U,
                           led->on ? pharmacy_rgb_to_color_name(led->red, led->green, led->blue) : "OFF");
        if (written < 0 || (size_t)written >= response_cap - used) return false;
        used += (size_t)written;
    }
    written = snprintf(response + used, response_cap - used, "\",\"led_list\":[");
    if (written < 0 || (size_t)written >= response_cap - used) return false;
    used += (size_t)written;
    for (size_t led_index = 0U; led_index < channel->led_count; ++led_index) {
        const pharmacy_led_assignment_t *led = &channel->leds[led_index];
        written = snprintf(response + used, response_cap - used,
                           "%s{\"led_no\":%zu,\"ledcolor\":\"%s\",\"team_id\":\"%s\"}",
                           led_index == 0U ? "" : ",", led_index + 1U,
                           led->on ? pharmacy_rgb_to_color_name(led->red, led->green, led->blue) : "OFF",
                           led->team_id[0] == '\0' ? "0" : led->team_id);
        if (written < 0 || (size_t)written >= response_cap - used) return false;
        used += (size_t)written;
    }
    written = snprintf(response + used, response_cap - used, "]}]");
    return written >= 0 && (size_t)written < response_cap - used;
}

int pharmacy_protocol_apply_led_control(pharmacy_protocol_context_t *ctx,
                                       const char *json_body,
                                       size_t body_len,
                                       pharmacy_response_t *response) {
    pharmacy_led_assignment_t parsed_assignments[PHARMACY_MAX_LEDS_PER_CHANNEL];
    pharmacy_led_assignment_t legacy_assignments[PHARMACY_MAX_LEDS_PER_CHANNEL];
    size_t parsed_count = 0U;
    size_t legacy_count = 0U;
    char channel_name[8];
    char status_name[8];
    bool has_led_list = false;
    bool has_legacy_leds = false;
    size_t i = 0U;

    if (ctx == NULL || response == NULL) {
        return -1;
    }
    pharmacy_response_reset(response);
    if (json_body == NULL || body_len == 0U) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "EMPTY_BODY");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "Request body is required");
        return -1;
    }
    if (body_len >= PHARMACY_MAX_JSON_BODY) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "REQUEST_TOO_LARGE");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "JSON body exceeds bounded limit");
        return -1;
    }
    if (!pharmacy_parse_string_json_field(json_body, "channel", channel_name, sizeof(channel_name))) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_CHANNEL");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "Missing or invalid channel");
        return -1;
    }
    if (!pharmacy_parse_string_json_field(json_body, "status", status_name, sizeof(status_name))) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_STATUS");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "Missing or invalid status");
        return -1;
    }

    if (strstr(json_body, "\"led_list\"") != NULL) {
        has_led_list = true;
        if (!pharmacy_parse_led_list_items(json_body, parsed_assignments, PHARMACY_MAX_LEDS_PER_CHANNEL, &parsed_count)) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_LED_LIST");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "led_list contains invalid or malformed entries");
            return -1;
        }
    }
    if (strstr(json_body, "\"leds\"") != NULL) {
        has_legacy_leds = true;
        if (!pharmacy_extract_legacy_leds(json_body, legacy_assignments, PHARMACY_MAX_LEDS_PER_CHANNEL, &legacy_count)) {
            snprintf(response->error_code, sizeof(response->error_code), "%s", "INVALID_LEDS");
            snprintf(response->error_message, sizeof(response->error_message), "%s", "leds string is malformed");
            return -1;
        }
    }

    if (has_led_list && has_legacy_leds) {
        for (i = 0U; i < legacy_count; ++i) {
            size_t j = 0U;
            for (j = 0U; j < parsed_count; ++j) {
                if (parsed_assignments[j].led_no == legacy_assignments[i].led_no &&
                    (parsed_assignments[j].red != legacy_assignments[i].red ||
                     parsed_assignments[j].green != legacy_assignments[i].green ||
                     parsed_assignments[j].blue != legacy_assignments[i].blue)) {
                    snprintf(response->error_code, sizeof(response->error_code), "%s", "CONFLICTING_LED_ASSIGNMENTS");
                    snprintf(response->error_message, sizeof(response->error_message), "%s", "led_list and leds disagree on the same LED");
                    return -1;
                }
            }
        }
    }

    if (strcmp(status_name, "off") == 0 && !has_led_list && !has_legacy_leds) {
        if (!pharmacy_apply_assignment_set(ctx, channel_name, parsed_assignments, 0u,
                                           status_name, response)) {
            return -1;
        }
        return 0;
    }
    if (!has_led_list && !has_legacy_leds) {
        snprintf(response->error_code, sizeof(response->error_code), "%s", "NO_LED_ASSIGNMENTS");
        snprintf(response->error_message, sizeof(response->error_message), "%s", "No valid LED assignments were supplied");
        return -1;
    }

    if (has_led_list) {
        if (!pharmacy_apply_assignment_set(ctx, channel_name, parsed_assignments, parsed_count, status_name, response)) {
            return -1;
        }
        return 0;
    }

    if (!pharmacy_apply_assignment_set(ctx, channel_name, legacy_assignments, legacy_count, status_name, response)) {
        return -1;
    }
    return 0;
}

int pharmacy_protocol_handle_request(pharmacy_protocol_context_t *ctx,
                                    const char *method,
                                    const char *path,
                                    const char *body,
                                    size_t body_len,
                                    char *response,
                                    size_t response_cap) {
    if (ctx == NULL || path == NULL || response == NULL || response_cap == 0U) {
        return 500;
    }
    if ((strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) &&
        !ctx->request_authorized) {
        snprintf(response, response_cap,
                 "{\"success\":false,\"error\":{\"code\":\"UNAUTHORIZED\","
                 "\"message\":\"Administrative authorization required\"}}");
        return 401;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/device/info") == 0) {
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"company\":\"%s\",\"product\":\"%s\",\"firmware_version\":\"%s\",\"hardware_revision\":\"%s\",\"ip_address\":\"%s\",\"mac_address\":\"%s\",\"uptime_seconds\":%u,\"network_status\":\"%s\",\"ota_status\":%s}",
                 ctx->device_info.company_name,
                 ctx->device_info.product_name,
                 ctx->device_info.firmware_version,
                 ctx->device_info.hardware_revision,
                 ctx->device_info.ip_address,
                 ctx->device_info.mac_address,
                 (unsigned int)ctx->device_info.uptime_seconds,
                 ctx->device_info.network_connected ? "up" : "down",
                 ctx->device_info.ota_active ? "true" : "false");
        return 200;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/channels") == 0) {
        size_t i = 0U;
        size_t written = 0U;
        char channel_json[256];
        char channel_list[1024];
        channel_list[0] = '\0';
        for (i = 0U; i < ctx->channel_count; ++i) {
            int active = 0;
            for (size_t j = 0U; j < ctx->channels[i].led_count; ++j) {
                if (ctx->channels[i].leds[j].on) {
                    ++active;
                }
            }
            snprintf(channel_json,
                     sizeof(channel_json),
                     "{\"channel\":\"%s\",\"status\":\"%s\",\"led_count\":%u,\"active_leds\":%d}",
                     ctx->channels[i].channel,
                     ctx->channels[i].enabled ? "on" : "off",
                     (unsigned int)ctx->channels[i].led_count,
                     active);
            if (channel_list[0] == '\0') {
                snprintf(channel_list, sizeof(channel_list), "[%s", channel_json);
            } else {
                snprintf(channel_list + written, sizeof(channel_list) - written, ",%s", channel_json);
            }
            written = strlen(channel_list);
        }
        if (channel_list[0] != '\0') {
            snprintf(channel_list + strlen(channel_list), sizeof(channel_list) - strlen(channel_list), "]");
        }
        snprintf(response, response_cap, "{\"success\":true,\"channels\":%s}", channel_list);
        return 200;
    }

    if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/channels/", strlen("/api/v1/channels/")) == 0) {
        char channel_name[8];
        size_t channel_index = 0U;
        const char *suffix = path + strlen("/api/v1/channels/");
        if (strlen(suffix) >= 3U) {
            memcpy(channel_name, suffix, 3u);
            channel_name[3] = '\0';
            if (pharmacy_channel_name_to_index(channel_name, &channel_index)) {
                snprintf(response,
                         response_cap,
                         "{\"success\":true,\"channel\":\"%s\",\"status\":\"%s\",\"led_count\":%u}",
                         ctx->channels[channel_index].channel,
                         ctx->channels[channel_index].enabled ? "on" : "off",
                         (unsigned int)ctx->channels[channel_index].led_count);
                return 200;
            }
        }
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/led/control") == 0) {
        pharmacy_response_t result;
        int rv = 0;
        pharmacy_response_reset(&result);
        rv = pharmacy_protocol_apply_led_control(ctx, body, body_len, &result);
        if (rv == 0) {
            snprintf(response, response_cap,
                     "{\"success\":true,\"channel\":\"%s\",\"status\":\"%s\",\"updated_leds\":%d,\"message\":\"%s\"}",
                     result.channel,
                     result.status,
                     result.updated_leds,
                     result.message);
            return 200;
        }
        snprintf(response, response_cap,
                 "{\"success\":false,\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
                 result.error_code,
                 result.error_message);
        return 400;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/picklight/ledcontrol") == 0) {
        pharmacy_response_t result;
        pharmacy_response_reset(&result);
        if (pharmacy_apply_picklight_control(ctx, body, &result) == 0) {
            snprintf(response, response_cap,
                     "{\"success\":true,\"channel\":\"%s\",\"status\":\"%s\",\"updated_leds\":%d,\"message\":\"%s\"}",
                     result.channel, result.status, result.updated_leds, result.message);
            return 200;
        }
        snprintf(response, response_cap,
                 "{\"success\":false,\"error\":{\"code\":\"INVALID_PICKLIGHT_REQUEST\",\"message\":\"location_id, teamcolor, team_id, status, and shelves are required\"}}");
        return 400;
    }

    if (strcmp(method, "GET") == 0 &&
        strncmp(path, "/api/v1/picklight/ledcontrol/", strlen("/api/v1/picklight/ledcontrol/")) == 0) {
        size_t channel_index = 0U;
        const char *channel_name = path + strlen("/api/v1/picklight/ledcontrol/");
        if (pharmacy_channel_name_to_index(channel_name, &channel_index) &&
            pharmacy_render_picklight_channel(ctx, channel_index, response, response_cap)) {
            return 200;
        }
        snprintf(response, response_cap,
                 "{\"success\":false,\"error\":{\"code\":\"INVALID_CHANNEL\",\"message\":\"Use C01 through C10\"}}");
        return 400;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/channels/all/off") == 0) {
        size_t i = 0U;
        size_t total = 0U;
        for (i = 0U; i < ctx->channel_count; ++i) {
            pharmacy_channel_state_t next = ctx->channels[i];
            next.enabled = false;
            for (size_t led_index = 0U; led_index < next.led_count; ++led_index) {
                next.leds[led_index].on = false;
            }
            if (ctx->led_commit != NULL &&
                !ctx->led_commit(ctx->led_commit_context, i, &next)) {
                snprintf(response, response_cap,
                         "{\"success\":false,\"error\":{\"code\":\"HARDWARE_COMMIT_FAILED\","
                         "\"message\":\"LED hardware rejected all-off command\"}}");
                return 503;
            }
            ctx->channels[i] = next;
            total += next.led_count;
        }
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"status\":\"off\",\"updated_leds\":%zu,\"message\":\"All channels disabled\"}",
                 total);
        return 200;
    }

    if (strcmp(method, "POST") == 0 && strncmp(path, "/api/v1/channels/", strlen("/api/v1/channels/")) == 0 && strstr(path, "/off") != NULL) {
        char channel_name[4];
        size_t channel_index = 0U;
        const char *component = path + strlen("/api/v1/channels/");
        if (strlen(component) != 7u || memcmp(component + 3u, "/off", 4u) != 0) {
            snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"INVALID_CHANNEL\",\"message\":\"Invalid channel identifier\"}}");
            return 400;
        }
        memcpy(channel_name, component, 3u);
        channel_name[3] = '\0';
        if (!pharmacy_channel_name_to_index(channel_name, &channel_index)) {
            snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"INVALID_CHANNEL\",\"message\":\"Invalid channel identifier\"}}");
            return 400;
        }
        pharmacy_channel_state_t next = ctx->channels[channel_index];
        next.enabled = false;
        for (size_t led_index = 0U; led_index < next.led_count; ++led_index) {
            next.leds[led_index].on = false;
        }
        if (ctx->led_commit != NULL &&
            !ctx->led_commit(ctx->led_commit_context, channel_index, &next)) {
            snprintf(response, response_cap,
                     "{\"success\":false,\"error\":{\"code\":\"HARDWARE_COMMIT_FAILED\","
                     "\"message\":\"LED hardware rejected off command\"}}");
            return 503;
        }
        ctx->channels[channel_index] = next;
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"channel\":\"%s\",\"status\":\"off\",\"updated_leds\":%u,\"message\":\"Channel disabled\"}",
                 ctx->channels[channel_index].channel,
                 (unsigned int)ctx->channels[channel_index].led_count);
        return 200;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/config") == 0) {
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"dhcp\":%s,\"ip\":\"%s\",\"subnet\":\"%s\",\"gateway\":\"%s\",\"dns\":\"%s\",\"http_port\":%d,\"device_name\":\"%s\"}",
                 ctx->network_config.dhcp_enabled ? "true" : "false",
                 ctx->network_config.ip_address,
                 ctx->network_config.subnet_mask,
                 ctx->network_config.gateway,
                 ctx->network_config.dns_server,
                 ctx->network_config.http_port,
                 ctx->network_config.device_name);
        return 200;
    }

    if (strcmp(method, "PUT") == 0 && strcmp(path, "/api/v1/config") == 0) {
        char dhcp_value[8];
        char ip_value[32];
        char subnet_value[32];
        char gateway_value[32];
        char dns_value[32];
        char device_name_value[32];
        char channel_value[8];
        int port_value = 0;
        int led_count_value = 0;
        int brightness_value = 0;
        int shelf_size_value = 0;
        bool recognized = false;
        pharmacy_network_config_t next = ctx->network_config;
        if (body == NULL || body_len == 0U) {
            snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"EMPTY_BODY\",\"message\":\"Required config JSON is missing\"}}");
            return 400;
        }
        if (pharmacy_parse_string_json_field(body, "channel", channel_value,
                                             sizeof(channel_value))) {
            size_t channel_index;
            if (!pharmacy_channel_name_to_index(channel_value, &channel_index) ||
                !pharmacy_parse_int_json_field(body, "led_count", &led_count_value) ||
                !pharmacy_parse_int_json_field(body, "brightness", &brightness_value) ||
                !pharmacy_parse_int_json_field(body, "leds_per_shelf", &shelf_size_value) ||
                led_count_value < 1 ||
                (unsigned int)led_count_value > PHARMACY_MAX_LEDS_PER_CHANNEL ||
                brightness_value < 0 || brightness_value > 255 ||
                shelf_size_value < 1 || shelf_size_value > led_count_value ||
                ctx->channel_config == NULL ||
                !ctx->channel_config(ctx->channel_config_context, channel_index,
                                     (uint16_t)led_count_value,
                                     (uint8_t)brightness_value,
                                     (uint8_t)shelf_size_value)) {
                snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"INVALID_CHANNEL_CONFIG\",\"message\":\"Channel configuration rejected\"}}");
                return 400;
            }
            ctx->channels[channel_index].led_count = (uint16_t)led_count_value;
            snprintf(response, response_cap, "{\"success\":true,\"message\":\"Channel configuration updated\"}");
            return 200;
        }
        if (pharmacy_parse_string_json_field(body, "dhcp", dhcp_value, sizeof(dhcp_value))) {
            next.dhcp_enabled = (strcasecmp(dhcp_value, "true") == 0 || strcasecmp(dhcp_value, "enabled") == 0);
            recognized = true;
        }
        if (pharmacy_parse_string_json_field(body, "ip", ip_value, sizeof(ip_value))) {
            snprintf(next.ip_address, sizeof(next.ip_address),
                     "%.15s", ip_value);
            recognized = true;
        }
        if (pharmacy_parse_string_json_field(body, "subnet", subnet_value, sizeof(subnet_value))) {
            snprintf(next.subnet_mask, sizeof(next.subnet_mask),
                     "%.15s", subnet_value);
            recognized = true;
        }
        if (pharmacy_parse_string_json_field(body, "gateway", gateway_value, sizeof(gateway_value))) {
            snprintf(next.gateway, sizeof(next.gateway),
                     "%.15s", gateway_value);
            recognized = true;
        }
        if (pharmacy_parse_string_json_field(body, "dns", dns_value, sizeof(dns_value))) {
            snprintf(next.dns_server, sizeof(next.dns_server),
                     "%.15s", dns_value);
            recognized = true;
        }
        if (pharmacy_parse_string_json_field(body, "device_name", device_name_value, sizeof(device_name_value))) {
            snprintf(next.device_name, sizeof(next.device_name), "%s", device_name_value);
            recognized = true;
        }
        if (pharmacy_parse_int_json_field(body, "http_port", &port_value)) {
            if (port_value < 1 || port_value > 65535) {
                snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"INVALID_CONFIG\",\"message\":\"HTTP port is out of range\"}}");
                return 400;
            }
            next.http_port = port_value;
            recognized = true;
        }
        if (!recognized) {
            snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"INVALID_CONFIG\",\"message\":\"No recognized configuration fields\"}}");
            return 400;
        }
        if (ctx->config_commit == NULL ||
            !ctx->config_commit(ctx->config_commit_context, &next)) {
            snprintf(response, response_cap, "{\"success\":false,\"error\":{\"code\":\"CONFIG_COMMIT_FAILED\",\"message\":\"Configuration validation or persistence failed\"}}");
            return 503;
        }
        ctx->network_config = next;
        snprintf(response, response_cap, "{\"success\":true,\"message\":\"Configuration updated\"}");
        return 200;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/health") == 0) {
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"status\":\"ok\",\"network\":%s,\"channels\":%u,\"uptime_seconds\":%u}",
                 ctx->device_info.network_connected ? "true" : "false",
                 (unsigned int)ctx->channel_count,
                 (unsigned int)ctx->device_info.uptime_seconds);
        return 200;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/ota/status") == 0) {
        snprintf(response,
                 response_cap,
                 "{\"success\":true,\"ota_active\":%s,\"status\":\"%s\"}",
                 ctx->ota_active ? "true" : "false",
                 ctx->ota_active ? "in_progress" : "idle");
        return 200;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/ota/upload") == 0) {
        snprintf(response, response_cap,
                 "{\"success\":false,\"error\":{\"code\":\"OTA_BACKEND_UNAVAILABLE\","
                 "\"message\":\"Verified board staging and signature backend not configured\"}}");
        return 503;
    }

    snprintf(response,
             response_cap,
             "{\"success\":false,\"error\":{\"code\":\"NOT_FOUND\",\"message\":\"Endpoint not found\"}}");
    return 404;
}
