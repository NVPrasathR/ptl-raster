#include "oled.h"

#include <string.h>

#if defined(PICO_ON_DEVICE) && OLED_ASSUME_SSD1306
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#endif

static oled_config_t oled_config;
static uint8_t framebuffer[OLED_MAX_WIDTH * OLED_MAX_HEIGHT / 8u];
static uint8_t next_page;
static bool initialized;
static bool dirty;

#if defined(PICO_ON_DEVICE) && OLED_ASSUME_SSD1306
static i2c_inst_t *oled_i2c(void) {
    return oled_config.i2c_instance == 0u ? i2c0 : i2c1;
}

static void send_command(uint8_t command) {
    uint8_t transfer[2] = {0x00u, command};
    (void)i2c_write_blocking(oled_i2c(), oled_config.i2c_address, transfer, sizeof(transfer), false);
}
#endif

oled_status_t oled_init(const oled_config_t *config) {
    if (config == NULL || config->width == 0u || config->width > OLED_MAX_WIDTH ||
        config->height == 0u || config->height > OLED_MAX_HEIGHT || (config->height % 8u) != 0u) {
        return OLED_STATUS_INVALID_ARGUMENT;
    }
#if !OLED_ASSUME_SSD1306
    (void)config;
    return OLED_STATUS_UNSUPPORTED_CONTROLLER;
#else
    oled_config = *config;
    memset(framebuffer, 0, sizeof(framebuffer));
    next_page = 0u;
    dirty = true;
    initialized = true;
#if defined(PICO_ON_DEVICE)
    i2c_init(oled_i2c(), 400000u);
    gpio_set_function(oled_config.sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(oled_config.scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(oled_config.sda_gpio);
    gpio_pull_up(oled_config.scl_gpio);
    send_command(0xaeu);
    send_command(0x20u);
    send_command(0x00u);
    send_command(0xafu);
#endif
    return OLED_STATUS_OK;
#endif
}

void oled_clear(void) {
    if (!initialized) return;
    memset(framebuffer, 0, sizeof(framebuffer));
    oled_mark_dirty();
}

bool oled_set_pixel(uint8_t x, uint8_t y, bool on) {
    uint16_t offset;
    uint8_t mask;
    if (!initialized || x >= oled_config.width || y >= oled_config.height) return false;
    offset = (uint16_t)x + (uint16_t)(y / 8u) * oled_config.width;
    mask = (uint8_t)(1u << (y % 8u));
    if (on) framebuffer[offset] |= mask;
    else framebuffer[offset] &= (uint8_t)~mask;
    dirty = true;
    return true;
}

void oled_mark_dirty(void) {
    if (initialized) dirty = true;
}

bool oled_is_dirty(void) {
    return dirty;
}

void oled_service(void) {
    if (!initialized || !dirty) return;
#if defined(PICO_ON_DEVICE) && OLED_ASSUME_SSD1306
    uint8_t transfer[OLED_MAX_WIDTH + 1u];
    uint8_t pages = (uint8_t)(oled_config.height / 8u);
    send_command((uint8_t)(0xb0u + next_page));
    send_command(0x00u);
    send_command(0x10u);
    transfer[0] = 0x40u;
    memcpy(&transfer[1], &framebuffer[(uint16_t)next_page * oled_config.width], oled_config.width);
    (void)i2c_write_blocking(oled_i2c(), oled_config.i2c_address, transfer, oled_config.width + 1u, false);
    ++next_page;
    if (next_page < pages) return;
#endif
    next_page = 0u;
    dirty = false;
}
