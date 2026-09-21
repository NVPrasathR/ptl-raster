#include "sh1106g.h"

#include <stdbool.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#define SH1106G_WIDTH 128u
#define SH1106G_HEIGHT 64u

static sh1106g_config_t config;
static uint8_t framebuffer[SH1106G_WIDTH * SH1106G_HEIGHT / 8u];
static bool initialized;

static i2c_inst_t *bus(void) {
    return config.i2c_instance == 0u ? i2c0 : i2c1;
}

static bool write_command(uint8_t command) {
    uint8_t packet[2] = {0x00u, command};
    return i2c_write_blocking(bus(), config.i2c_address, packet, sizeof(packet), false) == 2;
}

static void set_pixel(uint8_t x, uint8_t y) {
    if (x < config.width && y < config.height) {
        framebuffer[x + ((uint16_t)y / 8u) * config.width] |=
            (uint8_t)(1u << (y & 7u));
    }
}

static void glyph(char character, uint8_t glyph[5]) {
    static const uint8_t digits[10][5] = {
        {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
        {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
        {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e}
    };
    static const uint8_t letters[26][5] = {
        {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},
        {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
        {0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
        {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},
        {0x00,0x41,0x7f,0x41,0x00},{0x20,0x40,0x41,0x3f,0x01},
        {0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
        {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},
        {0x3e,0x41,0x41,0x41,0x3e},{0x7f,0x09,0x09,0x09,0x06},
        {0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},
        {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
        {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
        {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
    };
    memset(glyph, 0, 5);
    if (character >= '0' && character <= '9') memcpy(glyph, digits[character - '0'], 5);
    else if (character >= 'A' && character <= 'Z') memcpy(glyph, letters[character - 'A'], 5);
    else if (character >= 'a' && character <= 'z') memcpy(glyph, letters[character - 'a'], 5);
    else if (character == '-') glyph[2] = 0x08;
    else if (character == ':') { glyph[1] = 0x36; }
    else if (character == '.') glyph[2] = 0x40;
    else if (character == '/') { glyph[1] = 0x20; glyph[2] = 0x10; glyph[3] = 0x08; }
}

ptl_result_t sh1106g_init(const sh1106g_config_t *requested) {
    if (requested == NULL || requested->width != SH1106G_WIDTH ||
        requested->height != SH1106G_HEIGHT) return PTL_RESULT_INVALID_ARGUMENT;
    config = *requested;
    i2c_init(bus(), config.i2c_frequency_hz);
    gpio_set_function(config.sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(config.scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(config.sda_gpio);
    gpio_pull_up(config.scl_gpio);
    initialized = write_command(0xaeu) && write_command(0xd5u) &&
                  write_command(0x80u) && write_command(0xa8u) &&
                  write_command(0x3fu) && write_command(0xadu) &&
                  write_command(0x8bu) && write_command(0x20u) &&
                  write_command(0x02u) && write_command(0xafu);
    memset(framebuffer, 0, sizeof(framebuffer));
    return initialized ? PTL_RESULT_OK : PTL_RESULT_IO_ERROR;
}

void sh1106g_deinit(void) { initialized = false; }
void sh1106g_clear(void) { memset(framebuffer, 0, sizeof(framebuffer)); }

void sh1106g_draw_text(uint8_t x, uint8_t y, const char *text) {
    if (!initialized || text == NULL) return;
    while (*text != '\0' && x + 5u < config.width) {
        uint8_t columns[5];
        glyph(*text++, columns);
        for (uint8_t column = 0; column < 5u; ++column)
            for (uint8_t bit = 0; bit < 7u; ++bit)
                if ((columns[column] & (1u << bit)) != 0u) set_pixel(x + column, y + bit);
        x = (uint8_t)(x + 6u);
    }
}

void sh1106g_update(void) {
    if (!initialized) return;
    for (uint8_t page = 0; page < 8u; ++page) {
        uint8_t packet[129];
        if (!write_command((uint8_t)(0xb0u + page)) ||
            !write_command((uint8_t)(0x00u + (config.column_offset & 0x0fu))) ||
            !write_command((uint8_t)(0x10u | (config.column_offset >> 4u)))) return;
        packet[0] = 0x40u;
        memcpy(&packet[1], &framebuffer[(uint16_t)page * config.width], config.width);
        if (i2c_write_blocking(bus(), config.i2c_address, packet, sizeof(packet), false) != 129) return;
    }
}
