#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel_manager.h"
#include "color_manager.h"
#include "config_manager.h"
#include "flash_storage.h"
#include "host_verify_backend.h"
#include "http_server.h"
#include "ota_state.h"
#include "ota_verify.h"
#include "pharmacy_protocol.h"
#include "shelf_manager.h"
#include "w5500_eth.h"
#include "ws2812.h"

extern const uint8_t raster_dashboard_html_data[];
extern const size_t raster_dashboard_html_size;

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define TEST_STORAGE_SLOT_SIZE 16384u
#define RASTER_CONFIG_MAGIC_FOR_TEST 0x52434647u

typedef void (*test_fn_t)(void);

typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

typedef struct {
    uint32_t magic;
    uint32_t generation;
    uint32_t payload_size;
    uint32_t payload_crc;
    raster_config_t payload;
} test_config_record_t;

typedef struct {
    uint8_t slots[RASTER_STORAGE_SLOT_COUNT][TEST_STORAGE_SLOT_SIZE];
    bool read_ok;
    bool write_ok;
    bool erase_ok;
    unsigned writes;
    unsigned erases;
} fake_storage_t;

typedef struct {
    int auth_result;
    int verify_result;
    int write_result;
    int commit_result;
    int pending_result;
    int health_result;
    unsigned auth_calls;
    unsigned verify_calls;
    unsigned write_calls;
    unsigned commit_calls;
    unsigned pending_calls;
    unsigned health_calls;
    char expected_target[OTA_TARGET_LEN];
    char expected_version[OTA_VERSION_LEN];
    uint32_t expected_length;
} fake_ota_backend_t;

static int g_failures;
static fake_storage_t g_storage;
static bool g_led_commit_result = true;
static unsigned g_led_commit_calls;

static bool authorize_test_request(void *context, const char *credential) {
    const char *expected = (const char *)context;
    return expected != NULL && credential != NULL && strcmp(expected, credential) == 0;
}

static bool commit_test_leds(void *context, size_t channel,
                             const pharmacy_channel_state_t *state) {
    (void)context;
    (void)channel;
    (void)state;
    ++g_led_commit_calls;
    return g_led_commit_result;
}

static void fail_at(const char *file, int line, const char *expr) {
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    ++g_failures;
}

#define EXPECT_TRUE(expr) do { if (!(expr)) fail_at(__FILE__, __LINE__, #expr); } while (0)
#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))
#define EXPECT_EQ_U(actual, expected) do { unsigned long a_ = (unsigned long)(actual); unsigned long e_ = (unsigned long)(expected); if (a_ != e_) { fprintf(stderr, "%s:%d: expected %s=%lu got %s=%lu\n", __FILE__, __LINE__, #expected, e_, #actual, a_); ++g_failures; } } while (0)
#define EXPECT_EQ_I(actual, expected) do { long a_ = (long)(actual); long e_ = (long)(expected); if (a_ != e_) { fprintf(stderr, "%s:%d: expected %s=%ld got %s=%ld\n", __FILE__, __LINE__, #expected, e_, #actual, a_); ++g_failures; } } while (0)
#define EXPECT_STREQ(actual, expected) do { const char *a_ = (actual); const char *e_ = (expected); if (a_ == NULL || e_ == NULL || strcmp(a_, e_) != 0) { fprintf(stderr, "%s:%d: expected \"%s\" got \"%s\"\n", __FILE__, __LINE__, e_ == NULL ? "(null)" : e_, a_ == NULL ? "(null)" : a_); ++g_failures; } } while (0)
#define EXPECT_CONTAINS(haystack, needle) do { const char *h_ = (haystack); const char *n_ = (needle); if (h_ == NULL || n_ == NULL || strstr(h_, n_) == NULL) { fprintf(stderr, "%s:%d: expected substring \"%s\" in \"%s\"\n", __FILE__, __LINE__, n_ == NULL ? "(null)" : n_, h_ == NULL ? "(null)" : h_); ++g_failures; } } while (0)

static bool rgb_eq(ws2812_rgb_t actual, uint8_t red, uint8_t green, uint8_t blue) {
    return actual.red == red && actual.green == green && actual.blue == blue;
}

static bool bytes_contains(const uint8_t *haystack, size_t haystack_len, const char *needle) {
    size_t needle_len = needle == NULL ? 0u : strlen(needle);
    if (haystack == NULL || needle == NULL || needle_len == 0u || needle_len > haystack_len) {
        return false;
    }
    for (size_t offset = 0u; offset + needle_len <= haystack_len; ++offset) {
        if (memcmp(haystack + offset, needle, needle_len) == 0) {
            return true;
        }
    }
    return false;
}

static bool storage_read(uint32_t slot, void *destination, size_t length) {
    if (!g_storage.read_ok || slot >= RASTER_STORAGE_SLOT_COUNT || length > TEST_STORAGE_SLOT_SIZE) {
        return false;
    }
    memcpy(destination, g_storage.slots[slot], length);
    return true;
}

static bool storage_write(uint32_t slot, const void *source, size_t length) {
    if (!g_storage.write_ok || slot >= RASTER_STORAGE_SLOT_COUNT || length > TEST_STORAGE_SLOT_SIZE) {
        return false;
    }
    memcpy(g_storage.slots[slot], source, length);
    ++g_storage.writes;
    return true;
}

static bool storage_erase(uint32_t slot) {
    if (!g_storage.erase_ok || slot >= RASTER_STORAGE_SLOT_COUNT) {
        return false;
    }
    memset(g_storage.slots[slot], 0xff, sizeof(g_storage.slots[slot]));
    ++g_storage.erases;
    return true;
}

static void storage_reset(bool configured) {
    raster_storage_backend_t backend = {
        .read = storage_read,
        .write = storage_write,
        .erase = storage_erase,
        .slot_size = TEST_STORAGE_SLOT_SIZE,
    };
    memset(&g_storage, 0xff, sizeof(g_storage));
    g_storage.read_ok = true;
    g_storage.write_ok = true;
    g_storage.erase_ok = true;
    g_storage.writes = 0u;
    g_storage.erases = 0u;
    if (configured) {
        EXPECT_TRUE(raster_storage_set_backend(&backend));
    } else {
        EXPECT_FALSE(raster_storage_set_backend(NULL));
    }
}

static int fake_auth(void *ctx, const char *token, const char *target) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->auth_calls;
    if (token == NULL || target == NULL || strcmp(token, "token") != 0 ||
        strcmp(target, fake->expected_target) != 0) {
        return -1;
    }
    return fake->auth_result;
}

static int fake_verify(void *ctx, const char *target, const char *version, uint32_t length,
                       const uint8_t *digest, size_t digest_len) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->verify_calls;
    if (target == NULL || version == NULL || digest == NULL || digest_len != OTA_DIGEST_LEN ||
        strcmp(target, fake->expected_target) != 0 || strcmp(version, fake->expected_version) != 0 ||
        length != fake->expected_length) {
        return -1;
    }
    return fake->verify_result;
}

static int fake_verify_signature(void *ctx, const ota_image_meta_t *meta,
                                 const uint8_t *digest, size_t digest_len) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    if (fake == NULL || meta == NULL || digest == NULL ||
        digest_len != OTA_DIGEST_LEN || meta->signature_length == 0u) {
        return -1;
    }
    return memcmp(meta->digest, digest, digest_len) == 0 ? 0 : -1;
}

static int fake_stage_write(void *ctx, const uint8_t *data, size_t length, size_t offset) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->write_calls;
    if (data == NULL || length != fake->expected_length || offset != 0u) {
        return -1;
    }
    return fake->write_result;
}

static int fake_commit(void *ctx, const ota_image_meta_t *meta) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->commit_calls;
    return meta == NULL ? -1 : fake->commit_result;
}

static int fake_pending(void *ctx, const ota_image_meta_t *meta) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->pending_calls;
    return meta == NULL ? -1 : fake->pending_result;
}

static int fake_health(void *ctx, const ota_image_meta_t *meta) {
    fake_ota_backend_t *fake = (fake_ota_backend_t *)ctx;
    ++fake->health_calls;
    return meta == NULL ? -1 : fake->health_result;
}

static ota_backend_ops_t fake_backend_ops(fake_ota_backend_t *fake) {
    ota_backend_ops_t ops = {
        .authenticate = fake_auth,
        .verify_target = fake_verify,
        .verify_signature = fake_verify_signature,
        .staging_write = fake_stage_write,
        .stage_commit = fake_commit,
        .mark_pending = fake_pending,
        .confirm_health = fake_health,
        .ctx = fake,
    };
    return ops;
}

static void test_color_parsing_and_custom_rgb(void) {
    color_id_t color = COLOR_OFF;
    ws2812_rgb_t rgb = {0u, 0u, 0u};

    EXPECT_TRUE(color_manager_parse("red", &color));
    EXPECT_EQ_U(color, COLOR_RED);
    EXPECT_TRUE(color_manager_get(color, &rgb));
    EXPECT_TRUE(rgb_eq(rgb, 255u, 0u, 0u));
    EXPECT_TRUE(color_manager_parse("VIOLET", &color));
    EXPECT_EQ_U(color, COLOR_VIOLET);
    EXPECT_FALSE(color_manager_parse("magenta", &color));
    EXPECT_STREQ(color_manager_name(COLOR_YELLOW), "YELLOW");
    EXPECT_TRUE(color_manager_set_custom((ws2812_rgb_t){12u, 34u, 56u}));
    EXPECT_TRUE(color_manager_get(COLOR_CUSTOM, &rgb));
    EXPECT_TRUE(rgb_eq(rgb, 12u, 34u, 56u));
    EXPECT_STREQ(color_manager_name(COLOR_CUSTOM), "CUSTOM");
    EXPECT_FALSE(color_manager_get((color_id_t)99, &rgb));
}

static void test_channel_ids_led_bounds_and_configurable_counts(void) {
    size_t index = 99u;
    channel_info_t info;
    channel_led_state_t state;
    ws2812_rgb_t pixel;

    EXPECT_TRUE(channel_manager_init());
    for (unsigned channel = 1u; channel <= 10u; ++channel) {
        char name[4];
        snprintf(name, sizeof(name), "C%02u", channel);
        EXPECT_TRUE(pharmacy_protocol_parse_channel_name(name, &index));
        EXPECT_EQ_U(index, channel - 1u);
    }
    EXPECT_FALSE(pharmacy_protocol_parse_channel_name("C00", &index));
    EXPECT_FALSE(pharmacy_protocol_parse_channel_name("C11", &index));
    EXPECT_FALSE(pharmacy_protocol_parse_channel_name("CX1", &index));
    EXPECT_FALSE(pharmacy_protocol_parse_channel_name("1", &index));

    EXPECT_TRUE(channel_manager_get_info(0u, &info));
    EXPECT_EQ_U(info.led_count, WS2812_DEFAULT_LED_COUNT);
    EXPECT_TRUE(channel_manager_configure(2u, 4u, 99u, WS2812_WIRE_ORDER_RGB));
    EXPECT_TRUE(channel_manager_get_info(2u, &info));
    EXPECT_EQ_U(info.led_count, 4u);
    EXPECT_EQ_U(info.brightness, 99u);
    EXPECT_TRUE(channel_manager_set_led(2u, 3u, COLOR_GREEN, 7u, true));
    EXPECT_FALSE(channel_manager_set_led(2u, 4u, COLOR_GREEN, 7u, true));
    EXPECT_FALSE(channel_manager_set_led(10u, 0u, COLOR_GREEN, 7u, true));
    EXPECT_TRUE(channel_manager_get_led(2u, 3u, &state));
    EXPECT_EQ_U(state.color, COLOR_GREEN);
    EXPECT_EQ_U(state.team_id, 7u);
    EXPECT_TRUE(state.on);
    EXPECT_TRUE(ws2812_get_pixel(2u, 3u, &pixel) == WS2812_STATUS_OK);
    EXPECT_TRUE(rgb_eq(pixel, 0u, 255u, 0u));
    EXPECT_TRUE(channel_manager_set_enabled(2u, false));
    EXPECT_TRUE(ws2812_get_pixel(2u, 3u, &pixel) == WS2812_STATUS_OK);
    EXPECT_TRUE(rgb_eq(pixel, 0u, 0u, 0u));
    EXPECT_TRUE(channel_manager_configure(2u, WS2812_MAX_LEDS_PER_CHANNEL, 1u, WS2812_WIRE_ORDER_GRB));
    EXPECT_FALSE(channel_manager_configure(2u, (uint16_t)(WS2812_MAX_LEDS_PER_CHANNEL + 1u), 1u, WS2812_WIRE_ORDER_GRB));
}

static void test_shelf_partial_grouping(void) {
    uint16_t count = 0u;
    shelf_range_t range;
    channel_led_state_t state;

    EXPECT_TRUE(channel_manager_init());
    EXPECT_TRUE(channel_manager_configure(1u, 14u, 32u, WS2812_WIRE_ORDER_GRB));
    EXPECT_TRUE(shelf_manager_set_size(1u, 6u));
    EXPECT_TRUE(shelf_manager_get_count(1u, &count));
    EXPECT_EQ_U(count, 3u);
    EXPECT_TRUE(shelf_manager_get_range(1u, 0u, &range));
    EXPECT_EQ_U(range.first_led, 0u);
    EXPECT_EQ_U(range.led_count, 6u);
    EXPECT_TRUE(shelf_manager_get_range(1u, 2u, &range));
    EXPECT_EQ_U(range.first_led, 12u);
    EXPECT_EQ_U(range.led_count, 2u);
    EXPECT_FALSE(shelf_manager_get_range(1u, 3u, &range));
    EXPECT_TRUE(shelf_manager_set_color(1u, 2u, COLOR_BLUE, 5u, true));
    EXPECT_TRUE(channel_manager_get_led(1u, 12u, &state));
    EXPECT_EQ_U(state.team_id, 5u);
    EXPECT_EQ_U(state.color, COLOR_BLUE);
    EXPECT_TRUE(channel_manager_get_led(1u, 13u, &state));
    EXPECT_EQ_U(state.team_id, 5u);
    EXPECT_FALSE(shelf_manager_set_size(1u, 0u));
    EXPECT_FALSE(shelf_manager_set_size(99u, 1u));
}

static void test_protocol_json_legacy_precedence_atomic_and_status_off(void) {
    pharmacy_protocol_context_t ctx;
    pharmacy_response_t response;
    const char *strict_json =
        "{\"channel\":\"C03\",\"status\":\"on\",\"led_list\":["
        "{\"led_no\":1,\"ledcolor\":\"#123456\",\"team_id\":\"100\"},"
        "{\"led_no\":2,\"ledcolor\":\"GREEN\",\"team_id\":\"200\"}]}";
    const char *legacy_json = "{\"channel\":\"C04\",\"status\":\"on\",\"leds\":\"1:RED|2:BLUE\"}";
    const char *precedence_json =
        "{\"channel\":\"C05\",\"status\":\"on\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"YELLOW\",\"team_id\":\"300\"}],"
        "\"leds\":\"2:RED\"}";
    const char *conflict_json =
        "{\"channel\":\"C05\",\"status\":\"on\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"YELLOW\",\"team_id\":\"300\"}],"
        "\"leds\":\"1:RED\"}";
    char oversize[PHARMACY_MAX_JSON_BODY + 8u];

    pharmacy_protocol_init(&ctx);
    pharmacy_protocol_set_committers(&ctx, commit_test_leds, NULL, NULL, NULL,
                                     NULL, NULL);
    g_led_commit_calls = 0u;
    g_led_commit_result = true;
    EXPECT_TRUE(pharmacy_protocol_is_valid_team_id("123456"));
    EXPECT_FALSE(pharmacy_protocol_is_valid_team_id(""));
    EXPECT_FALSE(pharmacy_protocol_is_valid_team_id("team-1"));

    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, strict_json, strlen(strict_json), &response), 0);
    EXPECT_TRUE(response.successful);
    EXPECT_EQ_U(response.updated_leds, 2u);
    EXPECT_TRUE(ctx.channels[2].enabled);
    EXPECT_EQ_U(ctx.channels[2].leds[0].red, 0x12u);
    EXPECT_EQ_U(ctx.channels[2].leds[0].green, 0x34u);
    EXPECT_EQ_U(ctx.channels[2].leds[0].blue, 0x56u);
    EXPECT_STREQ(ctx.channels[2].leds[0].team_id, "100");
    EXPECT_STREQ(ctx.channels[2].leds[1].team_id, "200");
    EXPECT_EQ_U(g_led_commit_calls, 1u);

    g_led_commit_result = false;
    {
        const char *rejected =
            "{\"channel\":\"C03\",\"status\":\"on\",\"led_list\":["
            "{\"led_no\":3,\"ledcolor\":\"BLUE\",\"team_id\":\"300\"}]}";
        EXPECT_EQ_I(pharmacy_protocol_apply_led_control(
                        &ctx, rejected, strlen(rejected), &response), -1);
        EXPECT_STREQ(response.error_code, "HARDWARE_COMMIT_FAILED");
        EXPECT_FALSE(ctx.channels[2].leds[2].on);
    }
    g_led_commit_result = true;

    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, legacy_json, strlen(legacy_json), &response), 0);
    EXPECT_TRUE(ctx.channels[3].leds[0].on);
    EXPECT_STREQ(ctx.channels[3].leds[0].team_id, "0");

    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, precedence_json, strlen(precedence_json), &response), 0);
    EXPECT_TRUE(ctx.channels[4].leds[0].on);
    EXPECT_FALSE(ctx.channels[4].leds[1].on);
    EXPECT_EQ_U(ctx.channels[4].leds[0].red, 255u);
    EXPECT_EQ_U(ctx.channels[4].leds[0].green, 255u);

    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, conflict_json, strlen(conflict_json), &response), -1);
    EXPECT_STREQ(response.error_code, "CONFLICTING_LED_ASSIGNMENTS");

    {
        const char *duplicate =
            "{\"channel\":\"C03\",\"status\":\"on\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"RED\",\"team_id\":\"1\"},"
            "{\"led_no\":1,\"ledcolor\":\"BLUE\",\"team_id\":\"2\"}]}";
        EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, duplicate, strlen(duplicate), &response), -1);
        EXPECT_STREQ(response.error_code, "DUPLICATE_LED");
        EXPECT_STREQ(ctx.channels[2].leds[0].team_id, "100");
        EXPECT_EQ_U(ctx.channels[2].leds[0].red, 0x12u);
    }
    {
        const char *invalid_led =
            "{\"channel\":\"C03\",\"status\":\"on\",\"led_list\":[{\"led_no\":139,\"ledcolor\":\"RED\",\"team_id\":\"1\"}]}";
        EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, invalid_led, strlen(invalid_led), &response), -1);
        EXPECT_STREQ(response.error_code, "INVALID_LED_NUMBER");
        EXPECT_STREQ(ctx.channels[2].leds[0].team_id, "100");
    }
    {
        const char *malformed =
            "{\"channel\":\"C03\",\"status\":\"on\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"RED\"}]}";
        EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, malformed, strlen(malformed), &response), -1);
        EXPECT_STREQ(response.error_code, "INVALID_LED_LIST");
    }
    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, "", 0u, &response), -1);
    EXPECT_STREQ(response.error_code, "EMPTY_BODY");
    memset(oversize, ' ', sizeof(oversize));
    oversize[sizeof(oversize) - 1u] = '\0';
    EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, oversize, PHARMACY_MAX_JSON_BODY, &response), -1);
    EXPECT_STREQ(response.error_code, "REQUEST_TOO_LARGE");

    {
        const char *status_off =
            "{\"channel\":\"C03\",\"status\":\"off\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"RED\",\"team_id\":\"1\"}]}";
        EXPECT_EQ_I(pharmacy_protocol_apply_led_control(&ctx, status_off, strlen(status_off), &response), 0);
        EXPECT_FALSE(ctx.channels[2].enabled);
        EXPECT_FALSE(ctx.channels[2].leds[0].on);
        EXPECT_STREQ(ctx.channels[2].leds[0].team_id, "");
        EXPECT_EQ_U(response.updated_leds, 138u);
    }
}

static void test_http_parsing_routing_status_and_bounds(void) {
    pharmacy_protocol_context_t ctx;
    http_request_t request;
    char response[4096];
    int status = 0;
    const char *raw =
        "POST /api/v1/led/control HTTP/1.1\r\n"
        "Host: unit.test\r\n"
        "Authorization: Bearer unit-test\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"channel\":\"C01\",\"status\":\"on\",\"led_list\":[{\"led_no\":1,\"ledcolor\":\"RED\",\"team_id\":\"7\"}]}";

    pharmacy_protocol_init(&ctx);
    pharmacy_protocol_set_authorizer(&ctx, authorize_test_request, "Bearer unit-test");
    EXPECT_EQ_I(http_server_parse_request(raw, strlen(raw), &request), 0);
    EXPECT_STREQ(request.method, "POST");
    EXPECT_STREQ(request.path, "/api/v1/led/control");
    EXPECT_CONTAINS(request.body, "\"channel\":\"C01\"");
    EXPECT_STREQ(request.authorization, "Bearer unit-test");
    EXPECT_EQ_I(http_server_route_request(&ctx, &request, response, sizeof(response), &status), 0);
    EXPECT_EQ_I(status, 200);
    EXPECT_CONTAINS(response, "HTTP/1.1 200 OK");
    EXPECT_CONTAINS(response, "\"success\":true");

    request.authorization[0] = '\0';
    EXPECT_EQ_I(http_server_route_request(&ctx, &request, response, sizeof(response), &status), 0);
    EXPECT_EQ_I(status, 401);
    EXPECT_CONTAINS(response, "UNAUTHORIZED");

    memset(&request, 0, sizeof(request));
    snprintf(request.method, sizeof(request.method), "%s", "PATCH");
    snprintf(request.path, sizeof(request.path), "%s", "/api/v1/led/control");
    EXPECT_EQ_I(http_server_route_request(&ctx, &request, response, sizeof(response), &status), 0);
    EXPECT_EQ_I(status, 405);
    EXPECT_CONTAINS(response, "METHOD_NOT_ALLOWED");

    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "GET", "/api/v1/channels/C10", NULL, 0u, response, sizeof(response)), 200);
    EXPECT_CONTAINS(response, "\"channel\":\"C10\"");
    ctx.request_authorized = true;
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "POST", "/api/v1/channels/C11/off", NULL, 0u, response, sizeof(response)), 400);
    EXPECT_CONTAINS(response, "INVALID_CHANNEL");
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "GET", "/api/v1/nope", NULL, 0u, response, sizeof(response)), 404);
    EXPECT_CONTAINS(response, "NOT_FOUND");
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "GET", "/api/v1/health", NULL, 0u, response, sizeof(response)), 200);
    EXPECT_CONTAINS(response, "\"status\":\"ok\"");
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "GET", "/api/v1/ota/status", NULL, 0u, response, sizeof(response)), 200);
    EXPECT_CONTAINS(response, "\"status\":\"idle\"");
    ctx.request_authorized = true;
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "POST", "/api/v1/channels/off", NULL, 0u, response, sizeof(response)), 400);
    EXPECT_CONTAINS(response, "INVALID_CHANNEL");
    {
        char oversized[HTTP_SERVER_MAX_BODY + 256u];
        int length = snprintf(oversized, sizeof(oversized),
                              "POST /api/v1/led/control HTTP/1.1\r\n\r\n");
        memset(oversized + length, 'x', sizeof(oversized) - (size_t)length);
        EXPECT_EQ_I(http_server_parse_request(oversized, sizeof(oversized), &request), -1);
    }
    EXPECT_EQ_I(http_server_parse_request("GET / HTTP/1.1", strlen("GET / HTTP/1.1"), &request), -1);
}

static void test_config_validation_two_slot_crc_and_failure_recovery(void) {
    raster_config_t config;
    raster_config_t loaded;
    test_config_record_t *slot1 = (test_config_record_t *)g_storage.slots[1];

    storage_reset(false);
    EXPECT_EQ_I(raster_config_load(&loaded), RASTER_CONFIG_DEFAULTED);
    EXPECT_EQ_U(loaded.version, RASTER_CONFIG_VERSION);

    storage_reset(true);
    raster_config_defaults(&config);
    EXPECT_TRUE(raster_config_validate(&config));
    config.channels[0].led_count = 0u;
    EXPECT_FALSE(raster_config_validate(&config));
    raster_config_defaults(&config);
    config.channels[0].led_count = (uint16_t)(RASTER_MAX_LEDS_PER_CHANNEL + 1u);
    EXPECT_FALSE(raster_config_validate(&config));
    raster_config_defaults(&config);
    config.channels[0].leds_per_shelf = 0u;
    EXPECT_FALSE(raster_config_validate(&config));
    raster_config_defaults(&config);
    memset(config.network.device_name, 'A', sizeof(config.network.device_name));
    EXPECT_FALSE(raster_config_validate(&config));
    raster_config_defaults(&config);
    memset(config.network.mac, 0xff, sizeof(config.network.mac));
    EXPECT_FALSE(raster_config_validate(&config));

    raster_config_defaults(&config);
    config.channels[0].led_count = 111u;
    EXPECT_EQ_I(raster_config_save(&config), RASTER_CONFIG_OK);
    config.channels[0].led_count = 122u;
    config.network.address = (raster_ipv4_t){{10u, 0u, 0u, 42u}};
    EXPECT_EQ_I(raster_config_save(&config), RASTER_CONFIG_OK);
    EXPECT_EQ_I(raster_config_load(&loaded), RASTER_CONFIG_OK);
    EXPECT_EQ_U(loaded.channels[0].led_count, 122u);
    EXPECT_EQ_U(loaded.network.address.octet[3], 42u);
    EXPECT_EQ_U(slot1->magic, RASTER_CONFIG_MAGIC_FOR_TEST);
    slot1->payload.channels[0].led_count ^= 1u;
    EXPECT_EQ_I(raster_config_load(&loaded), RASTER_CONFIG_OK);
    EXPECT_EQ_U(loaded.channels[0].led_count, 111u);

    storage_reset(true);
    raster_config_defaults(&config);
    config.channels[0].led_count = 130u;
    EXPECT_EQ_I(raster_config_save(&config), RASTER_CONFIG_OK);
    config.channels[0].led_count = 140u;
    g_storage.write_ok = false;
    EXPECT_EQ_I(raster_config_save(&config), RASTER_CONFIG_STORAGE_ERROR);
    g_storage.write_ok = true;
    EXPECT_EQ_I(raster_config_load(&loaded), RASTER_CONFIG_OK);
    EXPECT_EQ_U(loaded.channels[0].led_count, 130u);
}

static void test_w5500_dhcp_static_and_recovery_state(void) {
    w5500_eth_context_t ctx;
    const uint8_t mac[6] = {0x02u, 0x11u, 0x22u, 0x33u, 0x44u, 0x55u};

    w5500_eth_init(&ctx, mac, true, NULL, NULL, NULL, NULL, "ptl", 8080);
    EXPECT_TRUE(ctx.initialized);
    EXPECT_TRUE(ctx.dhcp_enabled);
    EXPECT_STREQ(ctx.ip, "172.17.0.102");
    EXPECT_EQ_I(ctx.http_port, 8080);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "init");
    w5500_eth_update(&ctx, 100u, false);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "recovery");
    EXPECT_FALSE(w5500_eth_is_ready(&ctx));
    w5500_eth_update(&ctx, 1000u, true);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "recovery");
    w5500_eth_update(&ctx, 2200u, true);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "dhcp_wait");
    EXPECT_TRUE(w5500_eth_is_dhcp_active(&ctx));
    w5500_eth_update(&ctx, 2300u, true);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "dhcp_wait");
    EXPECT_TRUE(w5500_eth_is_ready(&ctx));

    w5500_eth_set_static_config(&ctx, "10.0.0.50", "255.255.255.0", "10.0.0.1", "8.8.8.8", 8081, false);
    EXPECT_FALSE(ctx.dhcp_enabled);
    EXPECT_STREQ(ctx.ip, "10.0.0.50");
    EXPECT_EQ_I(ctx.http_port, 8081);
    ctx.state = W5500_NET_STATE_INIT;
    w5500_eth_update(&ctx, 3000u, true);
    EXPECT_STREQ(w5500_eth_state_name(&ctx), "static_ready");
    EXPECT_TRUE(w5500_eth_is_ready(&ctx));
}

static void test_ota_digest_metadata_state_and_failures(void) {
    static const uint8_t image[] = {'a', 'b', 'c'};
    static const uint8_t test_signature[] = {0x01u};
    static const uint8_t expected_abc_sha256[OTA_DIGEST_LEN] = {
        0xbau, 0x78u, 0x16u, 0xbfu, 0x8fu, 0x01u, 0xcfu, 0xeau,
        0x41u, 0x41u, 0x40u, 0xdeu, 0x5du, 0xaeu, 0x22u, 0x23u,
        0xb0u, 0x03u, 0x61u, 0xa3u, 0x96u, 0x17u, 0x7au, 0x9cu,
        0xb4u, 0x10u, 0xffu, 0x61u, 0xf2u, 0x00u, 0x15u, 0xadu,
    };
    uint8_t digest[OTA_DIGEST_LEN];
    char hex[65];
    ota_session_t session;
    fake_ota_backend_t fake;
    ota_backend_ops_t ops;

    EXPECT_EQ_I(ota_digest_compute(image, sizeof(image), digest), 0);
    EXPECT_TRUE(memcmp(digest, expected_abc_sha256, sizeof(digest)) == 0);
    EXPECT_EQ_I(ota_verify_digest(image, sizeof(image), expected_abc_sha256), 0);
    digest[0] ^= 0xffu;
    EXPECT_EQ_I(ota_verify_digest(image, sizeof(image), digest), -1);
    ota_digest_to_hex(expected_abc_sha256, hex);
    EXPECT_STREQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    memset(&fake, 0, sizeof(fake));
    snprintf(fake.expected_target, sizeof(fake.expected_target), "%s", "rp2354");
    snprintf(fake.expected_version, sizeof(fake.expected_version), "%s", "1.2.3");
    fake.expected_length = (uint32_t)sizeof(image);
    ops = fake_backend_ops(&fake);

    ota_session_init(&session);
    EXPECT_EQ_I(ota_begin_image(&session, "rp2354", "1.2.3", sizeof(image)), -1);
    EXPECT_EQ_U(session.last_error, OTA_ERR_AUTH);
    EXPECT_EQ_I(ota_authenticate(&session, &ops, "bad", "rp2354"), -1);
    EXPECT_EQ_U(session.last_error, OTA_ERR_AUTH);
    EXPECT_EQ_I(ota_authenticate(&session, &ops, "token", "rp2354"), 0);
    EXPECT_EQ_U(session.state, OTA_STATE_READY);
    EXPECT_EQ_I(ota_begin_image(&session, "rp2354", "1.2.3", sizeof(image)), 0);
    EXPECT_EQ_I(ota_set_image_authenticity(&session, expected_abc_sha256,
                                           test_signature, sizeof(test_signature)), 0);
    EXPECT_EQ_U(session.state, OTA_STATE_RECEIVING);
    EXPECT_EQ_I(ota_stream_chunk(&session, image, 2u), 0);
    EXPECT_EQ_I(ota_finalize_upload(&session, &ops), -1);
    EXPECT_EQ_U(session.last_error, OTA_ERR_LENGTH);
    EXPECT_EQ_I(ota_stream_chunk(&session, image + 2u, 1u), 0);
    EXPECT_EQ_I(ota_finalize_upload(&session, &ops), 0);
    EXPECT_EQ_U(fake.verify_calls, 1u);
    EXPECT_EQ_U(fake.write_calls, 1u);
    EXPECT_EQ_U(fake.commit_calls, 1u);
    EXPECT_EQ_U(session.state, OTA_STATE_PENDING);
    EXPECT_STREQ(session.meta.digest_hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_EQ_I(ota_accept_pending(&session, &ops), 0);
    EXPECT_EQ_U(session.state, OTA_STATE_HEALTH_CONFIRM);
    EXPECT_EQ_I(ota_confirm_health(&session, &ops), 0);
    EXPECT_EQ_U(session.state, OTA_STATE_APPLIED);

    ota_session_reset(&session);
    EXPECT_EQ_U(session.state, OTA_STATE_IDLE);
    EXPECT_EQ_I(ota_authenticate(&session, &ops, "token", "rp2354"), 0);
    EXPECT_EQ_I(ota_begin_image(&session, "wrong", "1.2.3", sizeof(image)), 0);
    EXPECT_EQ_I(ota_set_image_authenticity(&session, expected_abc_sha256,
                                           test_signature, sizeof(test_signature)), 0);
    EXPECT_EQ_I(ota_stream_chunk(&session, image, sizeof(image)), 0);
    EXPECT_EQ_I(ota_finalize_upload(&session, &ops), -1);
    EXPECT_EQ_U(session.last_error, OTA_ERR_TARGET);

    ota_session_reset(&session);
    EXPECT_EQ_I(ota_authenticate(&session, &ops, "token", "rp2354"), 0);
    EXPECT_EQ_I(ota_begin_image(&session, "rp2354", "1.2.3", sizeof(image)), 0);
    EXPECT_EQ_I(ota_set_image_authenticity(&session, expected_abc_sha256,
                                           test_signature, sizeof(test_signature)), 0);
    EXPECT_EQ_I(ota_stream_chunk(&session, image, sizeof(image)), 0);
    EXPECT_EQ_I(ota_abort(&session, "interrupted"), 0);
    EXPECT_EQ_U(session.state, OTA_STATE_ABORTED);
    EXPECT_EQ_I(ota_finalize_upload(&session, &ops), -1);
    EXPECT_EQ_U(session.last_error, OTA_ERR_ABORTED);

    ota_session_reset(&session);
    fake.health_result = -1;
    EXPECT_EQ_I(ota_authenticate(&session, &ops, "token", "rp2354"), 0);
    EXPECT_EQ_I(ota_begin_image(&session, "rp2354", "1.2.3", sizeof(image)), 0);
    EXPECT_EQ_I(ota_set_image_authenticity(&session, expected_abc_sha256,
                                           test_signature, sizeof(test_signature)), 0);
    EXPECT_EQ_I(ota_stream_chunk(&session, image, sizeof(image)), 0);
    EXPECT_EQ_I(ota_finalize_upload(&session, &ops), 0);
    EXPECT_EQ_I(ota_accept_pending(&session, &ops), 0);
    EXPECT_EQ_I(ota_confirm_health(&session, &ops), -1);
    EXPECT_EQ_U(session.state, OTA_STATE_FAILED);

    ota_session_reset(&session);
}

static void test_protocol_network_sync_uses_live_ip(void) {
    pharmacy_protocol_context_t ctx;

    pharmacy_protocol_init(&ctx);
    EXPECT_STREQ(ctx.device_info.ip_address, "172.17.0.102");
    EXPECT_TRUE(ctx.device_info.network_connected);

    pharmacy_protocol_sync_network(&ctx, "172.17.0.102", true, true);
    EXPECT_STREQ(ctx.device_info.ip_address, "172.17.0.102");
    EXPECT_STREQ(ctx.network_config.ip_address, "172.17.0.102");
    EXPECT_TRUE(ctx.device_info.network_connected);
    EXPECT_TRUE(ctx.network_config.dhcp_enabled);

    pharmacy_protocol_sync_network(&ctx, "10.0.0.20", false, false);
    EXPECT_STREQ(ctx.device_info.ip_address, "10.0.0.20");
    EXPECT_FALSE(ctx.device_info.network_connected);
    EXPECT_FALSE(ctx.network_config.dhcp_enabled);
}

static void test_picklight_compatibility_routes(void) {
    pharmacy_protocol_context_t ctx;
    char response[PHARMACY_JSON_RESPONSE_CAPACITY];
    const char *body =
        "{\"teamcolor\":\"RED\",\"team_id\":\"1\",\"status\":\"on\","
        "\"location_id\":4,\"shelves\":[{\"shelf_phr_id\":59,\"shelf_name\":\"A\"},"
        "{\"shelf_phr_id\":60,\"shelf_name\":\"B\"}]}";

    pharmacy_protocol_init(&ctx);
    ctx.request_authorized = true;
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "POST", "/api/v1/picklight/ledcontrol",
                                                 body, strlen(body), response, sizeof(response)), 200);
    EXPECT_CONTAINS(response, "\"channel\":\"C04\"");
    EXPECT_CONTAINS(response, "\"updated_leds\":12");
    EXPECT_EQ_I(pharmacy_protocol_handle_request(&ctx, "GET", "/api/v1/picklight/ledcontrol/C04",
                                                 NULL, 0u, response, sizeof(response)), 200);
    EXPECT_CONTAINS(response, "\"channel\":\"C04\"");
    EXPECT_CONTAINS(response, "1:RED|2:RED|3:RED|4:RED|5:RED|6:RED|");
    EXPECT_CONTAINS(response, "\"led_no\":12,\"ledcolor\":\"RED\",\"team_id\":\"1\"");
}

static void test_dashboard_assets_present(void) {
    EXPECT_TRUE(raster_dashboard_html_data != NULL);
    EXPECT_TRUE(raster_dashboard_html_size > 1024u);
    EXPECT_TRUE(bytes_contains(raster_dashboard_html_data, raster_dashboard_html_size, "Firmware Upload"));
    EXPECT_TRUE(bytes_contains(raster_dashboard_html_data, raster_dashboard_html_size, "/api/v1/led/control"));
    EXPECT_TRUE(bytes_contains(raster_dashboard_html_data, raster_dashboard_html_size, "/api/v1/ota/upload"));
}

int main(void) {
    const test_case_t tests[] = {
        {"color parsing and custom RGB", test_color_parsing_and_custom_rgb},
        {"channel IDs, LED bounds, configurable counts", test_channel_ids_led_bounds_and_configurable_counts},
        {"shelf partial grouping", test_shelf_partial_grouping},
        {"strict JSON, legacy parsing, precedence, atomic failure, status off", test_protocol_json_legacy_precedence_atomic_and_status_off},
        {"HTTP parsing, routing, status, bounds", test_http_parsing_routing_status_and_bounds},
        {"configuration validation, two-slot persistence, CRC, recovery", test_config_validation_two_slot_crc_and_failure_recovery},
        {"W5500 DHCP, static, recovery state", test_w5500_dhcp_static_and_recovery_state},
        {"protocol network sync uses live IP", test_protocol_network_sync_uses_live_ip},
        {"external pick-light compatibility routes", test_picklight_compatibility_routes},
        {"OTA digest, metadata, state, auth, failure paths", test_ota_digest_metadata_state_and_failures},
        {"dashboard assets present", test_dashboard_assets_present},
    };

    for (size_t i = 0u; i < ARRAY_LEN(tests); ++i) {
        int before = g_failures;
        printf("RUN %s\n", tests[i].name);
        tests[i].fn();
        printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", tests[i].name);
    }

    if (g_failures != 0) {
        fprintf(stderr, "%d assertion failure(s)\n", g_failures);
        return EXIT_FAILURE;
    }
    printf("PASS all %zu host test groups\n", ARRAY_LEN(tests));
    return EXIT_SUCCESS;
}
