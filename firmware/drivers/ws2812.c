#include "ws2812.h"

#include <string.h>

#if defined(PICO_ON_DEVICE)
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"
#endif

typedef struct {
    ws2812_channel_config_t config;
    ws2812_rgb_t pixels[WS2812_MAX_LEDS_PER_CHANNEL];
    bool dirty;
} ws2812_channel_state_t;

static ws2812_channel_state_t channels[WS2812_CHANNEL_COUNT];
static uint8_t next_channel;

#if defined(PICO_ON_DEVICE)
static PIO tx_pio = pio0;
static uint tx_sm;
static int tx_dma = -1;
static uint tx_program_offset;
static uint32_t tx_words[WS2812_MAX_LEDS_PER_CHANNEL];
static bool tx_active;
static uint32_t tx_ready_at_us;
#endif

static bool valid_channel(uint8_t channel) {
    return channel < WS2812_CHANNEL_COUNT;
}

#if defined(PICO_ON_DEVICE)
static uint32_t pack_pixel(ws2812_rgb_t color, ws2812_wire_order_t order, uint8_t brightness) {
    uint8_t red = (uint8_t)(((uint16_t)color.red * brightness) / UINT8_MAX);
    uint8_t green = (uint8_t)(((uint16_t)color.green * brightness) / UINT8_MAX);
    uint8_t blue = (uint8_t)(((uint16_t)color.blue * brightness) / UINT8_MAX);

    switch (order) {
    case WS2812_WIRE_ORDER_RGB: return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
    case WS2812_WIRE_ORDER_BRG: return ((uint32_t)blue << 16) | ((uint32_t)red << 8) | green;
    case WS2812_WIRE_ORDER_BGR: return ((uint32_t)blue << 16) | ((uint32_t)green << 8) | red;
    case WS2812_WIRE_ORDER_RBG: return ((uint32_t)red << 16) | ((uint32_t)blue << 8) | green;
    case WS2812_WIRE_ORDER_GBR: return ((uint32_t)green << 16) | ((uint32_t)blue << 8) | red;
    case WS2812_WIRE_ORDER_GRB:
    default: return ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    }
}
#endif

void ws2812_init(void) {
    for (uint8_t channel = 0; channel < WS2812_CHANNEL_COUNT; ++channel) {
        channels[channel].config.gpio = channel;
        channels[channel].config.led_count = WS2812_DEFAULT_LED_COUNT;
        channels[channel].config.brightness = 32u;
        channels[channel].config.wire_order = WS2812_WIRE_ORDER_GRB;
        memset(channels[channel].pixels, 0, sizeof(channels[channel].pixels));
        channels[channel].dirty = true;
    }
#if defined(PICO_ON_DEVICE)
    tx_program_offset = pio_add_program(tx_pio, &ws2812_program);
    tx_sm = pio_claim_unused_sm(tx_pio, true);
    tx_dma = dma_claim_unused_channel(true);
#endif
}

ws2812_status_t ws2812_configure_channel(uint8_t channel, const ws2812_channel_config_t *config) {
    if (!valid_channel(channel) || config == NULL || config->led_count == 0u ||
        config->led_count > WS2812_MAX_LEDS_PER_CHANNEL || config->wire_order > WS2812_WIRE_ORDER_GBR) {
        return WS2812_STATUS_INVALID_ARGUMENT;
    }
    channels[channel].config = *config;
    channels[channel].dirty = true;
    return WS2812_STATUS_OK;
}

ws2812_status_t ws2812_set_pixel(uint8_t channel, uint16_t index, ws2812_rgb_t color) {
    if (!valid_channel(channel) || index >= channels[channel].config.led_count) {
        return WS2812_STATUS_OUT_OF_RANGE;
    }
    channels[channel].pixels[index] = color;
    channels[channel].dirty = true;
    return WS2812_STATUS_OK;
}

ws2812_status_t ws2812_get_pixel(uint8_t channel, uint16_t index, ws2812_rgb_t *color) {
    if (!valid_channel(channel) || color == NULL || index >= channels[channel].config.led_count) {
        return WS2812_STATUS_OUT_OF_RANGE;
    }
    *color = channels[channel].pixels[index];
    return WS2812_STATUS_OK;
}

ws2812_status_t ws2812_clear_channel(uint8_t channel) {
    if (!valid_channel(channel)) {
        return WS2812_STATUS_OUT_OF_RANGE;
    }
    memset(channels[channel].pixels, 0, sizeof(channels[channel].pixels));
    channels[channel].dirty = true;
    return WS2812_STATUS_OK;
}

void ws2812_mark_dirty(uint8_t channel) {
    if (valid_channel(channel)) {
        channels[channel].dirty = true;
    }
}

bool ws2812_channel_dirty(uint8_t channel) {
    return valid_channel(channel) && channels[channel].dirty;
}

void ws2812_service(void) {
#if defined(PICO_ON_DEVICE)
    if (tx_active) {
        if (dma_channel_is_busy(tx_dma)) {
            return;
        }
        tx_active = false;
        tx_ready_at_us = time_us_32() + 300u;
    }
    if ((int32_t)(time_us_32() - tx_ready_at_us) < 0) {
        return;
    }
#endif
    for (uint8_t offset = 0; offset < WS2812_CHANNEL_COUNT; ++offset) {
        uint8_t channel = (uint8_t)((next_channel + offset) % WS2812_CHANNEL_COUNT);
        if (!channels[channel].dirty) {
            continue;
        }
#if defined(PICO_ON_DEVICE)
        for (uint16_t index = 0; index < channels[channel].config.led_count; ++index) {
            tx_words[index] = pack_pixel(channels[channel].pixels[index], channels[channel].config.wire_order,
                                         channels[channel].config.brightness) << 8;
        }
        pio_sm_set_enabled(tx_pio, tx_sm, false);
        pio_sm_config config = ws2812_program_get_default_config(tx_program_offset);
        sm_config_set_out_pins(&config, channels[channel].config.gpio, 1u);
        sm_config_set_sideset_pins(&config, channels[channel].config.gpio);
        sm_config_set_out_shift(&config, false, true, 24u);
        sm_config_set_clkdiv(&config, (float)clock_get_hz(clk_sys) / 8000000.0f);
        pio_gpio_init(tx_pio, channels[channel].config.gpio);
        pio_sm_set_consecutive_pindirs(tx_pio, tx_sm, channels[channel].config.gpio, 1u, true);
        pio_sm_init(tx_pio, tx_sm, tx_program_offset, &config);
        dma_channel_config dma_config = dma_channel_get_default_config(tx_dma);
        channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
        channel_config_set_dreq(&dma_config, pio_get_dreq(tx_pio, tx_sm, true));
        dma_channel_configure(tx_dma, &dma_config, &tx_pio->txf[tx_sm], tx_words,
                              channels[channel].config.led_count, true);
        pio_sm_set_enabled(tx_pio, tx_sm, true);
        tx_active = true;
#endif
        channels[channel].dirty = false;
        next_channel = (uint8_t)((channel + 1u) % WS2812_CHANNEL_COUNT);
        return;
    }
}
