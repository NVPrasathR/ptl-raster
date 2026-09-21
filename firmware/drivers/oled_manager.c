#include "oled_manager.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board_pins.h"
#include "sh1106g.h"

#define OLED_WIDTH 128u
#define OLED_HEIGHT 64u
#define OLED_ADDRESS 0x3cu
#define OLED_I2C_HZ 400000u

static bool initialized;
static bool dirty;
static oled_network_state_t network_state;
static oled_message_t message;
static uint32_t ip_address;
static uint8_t diagnostic_state;
static uint32_t diagnostic_error;
static uint8_t ota_progress;
static char detail[16];
static char firmware_version[16] = "1.0.0";
static char error_text[24];

static void line(uint8_t number, const char *text) {
    sh1106g_draw_text(0u, (uint8_t)(number * 10u), text);
}

static void render(void) {
    char buffer[32];
    sh1106g_clear();
    if (message != OLED_MESSAGE_NONE) {
        static const char *const labels[] = {
            "", "CONFIG SAVED", "OTA UPDATE", "OTA SUCCESS", "OTA FAILED",
            "FACTORY RESET", "NETWORK ERROR", "SYSTEM ERROR"
        };
        line(0u, labels[message <= OLED_MESSAGE_SYSTEM_ERROR ? message : 0u]);
        if (message == OLED_MESSAGE_SYSTEM_ERROR && error_text[0] != '\0') line(2u, error_text);
        if (ota_progress != 0u) {
            (void)snprintf(buffer, sizeof(buffer), "OTA %u%%", ota_progress);
            line(4u, buffer);
        }
        return;
    }
    line(0u, "RASTER");
    line(1u, "PICK TO LIGHT");
    (void)snprintf(buffer, sizeof(buffer), "IP: %u.%u.%u.%u",
                   (unsigned)((ip_address >> 24u) & 0xffu),
                   (unsigned)((ip_address >> 16u) & 0xffu),
                   (unsigned)((ip_address >> 8u) & 0xffu),
                   (unsigned)(ip_address & 0xffu));
    line(2u, ip_address != 0u ? buffer :
          (network_state == OLED_NETWORK_DHCP ? "IP: DHCP" : "IP: --"));
    line(3u, detail[0] != '\0' ? detail :
          (network_state == OLED_NETWORK_DISCONNECTED ? "NO LINK" : ""));
    (void)snprintf(buffer, sizeof(buffer), "NET:%u ERR:%lu",
                   (unsigned)diagnostic_state, (unsigned long)diagnostic_error);
    line(4u, buffer);
    (void)snprintf(buffer, sizeof(buffer), "FW: %s", firmware_version);
    line(5u, buffer);
}

ptl_result_t oled_manager_init(void) {
    const sh1106g_config_t config = {
        .i2c_instance = 0u, .i2c_address = OLED_ADDRESS,
        .sda_gpio = RASTER_OLED_SDA_PIN, .scl_gpio = RASTER_OLED_SCL_PIN,
        .i2c_frequency_hz = OLED_I2C_HZ, .width = OLED_WIDTH, .height = OLED_HEIGHT,
        .column_offset = 2u, .orientation = 0u
    };
    initialized = false;
    message = OLED_MESSAGE_NONE;
    network_state = OLED_NETWORK_UNKNOWN;
    ip_address = 0u;
    diagnostic_state = 0u;
    diagnostic_error = 0u;
    ota_progress = 0u;
    detail[0] = '\0';
    error_text[0] = '\0';
    if (sh1106g_init(&config) != PTL_RESULT_OK) return PTL_RESULT_IO_ERROR;
    initialized = true;
    dirty = true;
    return PTL_RESULT_OK;
}

void oled_manager_deinit(void) { initialized = false; sh1106g_deinit(); }
void oled_manager_service(void) {
    if (initialized && dirty) { render(); sh1106g_update(); dirty = false; }
}
void oled_manager_set_network_state(oled_network_state_t state) { network_state = state; dirty = true; }
void oled_manager_set_network_diagnostic(uint8_t state, uint32_t error_code) {
    diagnostic_state = state; diagnostic_error = error_code; dirty = true;
}
void oled_manager_set_network_detail(const char *value) {
    if (value != NULL) { (void)snprintf(detail, sizeof(detail), "%s", value); dirty = true; }
}
void oled_manager_set_ip(uint32_t value) { ip_address = value; dirty = true; }
void oled_manager_set_fw_version(const char *value) {
    if (value != NULL) { (void)snprintf(firmware_version, sizeof(firmware_version), "%s", value); dirty = true; }
}
void oled_manager_show_message(oled_message_t value) { message = value; dirty = true; }
void oled_manager_show_ota_progress(uint8_t value) { ota_progress = value > 100u ? 100u : value; dirty = true; }
void oled_manager_show_error(const char *value) {
    if (value != NULL) { (void)snprintf(error_text, sizeof(error_text), "%s", value); message = OLED_MESSAGE_SYSTEM_ERROR; dirty = true; }
}
