#include "config_manager.h"

#include "flash_storage.h"

#include <string.h>

#define RASTER_CONFIG_MAGIC 0x52434647u
#define RASTER_DEFAULT_BRIGHTNESS 32u
#define RASTER_DEFAULT_BOOT_BEEP_MS 100u

typedef struct {
    uint32_t magic;
    uint32_t generation;
    uint32_t payload_size;
    uint32_t payload_crc;
    raster_config_t payload;
} raster_config_record_t;

static bool string_terminated(const char *value, size_t capacity) {
    return value != NULL && memchr(value, '\0', capacity) != NULL;
}

static bool ipv4_is_unicast_or_zero(raster_ipv4_t address) {
    if (address.octet[0] == 0u && address.octet[1] == 0u &&
        address.octet[2] == 0u && address.octet[3] == 0u) {
        return true;
    }
    return address.octet[0] < 224u && address.octet[0] != 127u &&
           address.octet[0] != 0u && address.octet[3] != 255u;
}

static bool record_valid(const raster_config_record_t *record) {
    return record != NULL && record->magic == RASTER_CONFIG_MAGIC &&
           record->payload_size == sizeof(record->payload) &&
           record->payload_crc ==
               raster_config_crc32(&record->payload, sizeof(record->payload)) &&
           raster_config_validate(&record->payload);
}

static bool read_record(uint32_t slot, raster_config_record_t *record) {
    memset(record, 0, sizeof(*record));
    return raster_storage_read(slot, record, sizeof(*record)) && record_valid(record);
}

void raster_config_defaults(raster_config_t *config) {
    static const uint8_t default_mac[6] = {0x02u, 0x52u, 0x41u, 0x53u, 0x54u, 0x01u};
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->version = RASTER_CONFIG_VERSION;
    config->network.dhcp_enabled = true;
    config->network.address = (raster_ipv4_t){{192u, 168u, 1u, 250u}};
    config->network.netmask = (raster_ipv4_t){{255u, 255u, 255u, 0u}};
    config->network.gateway = (raster_ipv4_t){{192u, 168u, 1u, 1u}};
    config->network.dns = (raster_ipv4_t){{192u, 168u, 1u, 1u}};
    config->network.http_port = 80u;
    memcpy(config->network.device_name, "raster-pick-to-light",
           sizeof("raster-pick-to-light"));
    memcpy(config->network.mac, default_mac, sizeof(default_mac));

    for (uint32_t index = 0; index < RASTER_CHANNEL_COUNT; ++index) {
        raster_channel_config_t *channel = &config->channels[index];
        channel->led_count = RASTER_DEFAULT_LED_COUNT;
        channel->leds_per_shelf = RASTER_DEFAULT_LEDS_PER_SHELF;
        channel->brightness = RASTER_DEFAULT_BRIGHTNESS;
        channel->enabled = true;
        channel->name[0] = 'C';
        channel->name[1] = (char)('0' + ((index + 1u) / 10u));
        channel->name[2] = (char)('0' + ((index + 1u) % 10u));
        channel->name[3] = '\0';
    }

    config->buzzer_enabled = true;
    config->boot_beep_ms = RASTER_DEFAULT_BOOT_BEEP_MS;
    config->status_led_active_high = true;
    config->ws2812_color_order = 0u;
}

bool raster_config_validate(const raster_config_t *config) {
    if (config == NULL || config->version != RASTER_CONFIG_VERSION ||
        config->network.http_port == 0u ||
        !string_terminated(config->network.device_name, sizeof(config->network.device_name)) ||
        !ipv4_is_unicast_or_zero(config->network.address) ||
        !ipv4_is_unicast_or_zero(config->network.gateway) ||
        !ipv4_is_unicast_or_zero(config->network.dns) ||
        config->boot_beep_ms > 5000u || config->ws2812_color_order > 5u) {
        return false;
    }

    bool mac_nonzero = false;
    bool mac_all_ff = true;
    for (size_t index = 0; index < sizeof(config->network.mac); ++index) {
        mac_nonzero |= config->network.mac[index] != 0u;
        mac_all_ff &= config->network.mac[index] == 0xffu;
    }
    if (!mac_nonzero || mac_all_ff || (config->network.mac[0] & 1u) != 0u) {
        return false;
    }

    for (uint32_t index = 0; index < RASTER_CHANNEL_COUNT; ++index) {
        const raster_channel_config_t *channel = &config->channels[index];
        if (channel->led_count == 0u ||
            channel->led_count > RASTER_MAX_LEDS_PER_CHANNEL ||
            channel->leds_per_shelf == 0u ||
            channel->leds_per_shelf > channel->led_count ||
            !string_terminated(channel->name, sizeof(channel->name))) {
            return false;
        }
        for (uint32_t shelf = 0; shelf < RASTER_MAX_SHELF_LABELS; ++shelf) {
            if (!string_terminated(channel->shelf_labels[shelf],
                                   sizeof(channel->shelf_labels[shelf]))) {
                return false;
            }
        }
    }
    return true;
}

raster_config_result_t raster_config_load(raster_config_t *config) {
    raster_config_record_t slots[RASTER_STORAGE_SLOT_COUNT];
    bool valid[RASTER_STORAGE_SLOT_COUNT] = {false, false};
    if (config == NULL) {
        return RASTER_CONFIG_INVALID_ARGUMENT;
    }
    if (!raster_storage_is_configured()) {
        raster_config_defaults(config);
        return RASTER_CONFIG_DEFAULTED;
    }

    for (uint32_t slot = 0; slot < RASTER_STORAGE_SLOT_COUNT; ++slot) {
        valid[slot] = read_record(slot, &slots[slot]);
    }
    if (!valid[0] && !valid[1]) {
        raster_config_defaults(config);
        return RASTER_CONFIG_DEFAULTED;
    }

    uint32_t selected = valid[1] &&
                                (!valid[0] ||
                                 (int32_t)(slots[1].generation - slots[0].generation) > 0)
                            ? 1u
                            : 0u;
    *config = slots[selected].payload;
    return RASTER_CONFIG_OK;
}

raster_config_result_t raster_config_save(const raster_config_t *config) {
    raster_config_record_t current[RASTER_STORAGE_SLOT_COUNT];
    bool valid[RASTER_STORAGE_SLOT_COUNT] = {false, false};
    if (config == NULL) {
        return RASTER_CONFIG_INVALID_ARGUMENT;
    }
    if (!raster_config_validate(config)) {
        return RASTER_CONFIG_INVALID_VALUE;
    }
    if (!raster_storage_is_configured()) {
        return RASTER_CONFIG_STORAGE_ERROR;
    }

    for (uint32_t slot = 0; slot < RASTER_STORAGE_SLOT_COUNT; ++slot) {
        valid[slot] = read_record(slot, &current[slot]);
    }
    uint32_t active = valid[1] &&
                              (!valid[0] ||
                               (int32_t)(current[1].generation - current[0].generation) > 0)
                          ? 1u
                          : 0u;
    uint32_t target = valid[0] || valid[1] ? active ^ 1u : 0u;
    uint32_t generation = valid[active] ? current[active].generation + 1u : 1u;
    raster_config_record_t next = {
        .magic = RASTER_CONFIG_MAGIC,
        .generation = generation,
        .payload_size = sizeof(next.payload),
        .payload_crc = raster_config_crc32(config, sizeof(*config)),
        .payload = *config,
    };

    if (!raster_storage_erase(target) ||
        !raster_storage_write(target, &next, sizeof(next))) {
        return RASTER_CONFIG_STORAGE_ERROR;
    }
    raster_config_record_t verify;
    if (!read_record(target, &verify) || verify.generation != generation) {
        return RASTER_CONFIG_STORAGE_ERROR;
    }
    return RASTER_CONFIG_OK;
}

raster_config_result_t raster_config_factory_reset(raster_config_t *config) {
    if (config == NULL) {
        return RASTER_CONFIG_INVALID_ARGUMENT;
    }
    raster_config_defaults(config);
    return raster_config_save(config);
}

uint32_t raster_config_crc32(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xffffffffu;
    if (bytes == NULL && length != 0u) {
        return 0u;
    }
    for (size_t offset = 0; offset < length; ++offset) {
        crc ^= bytes[offset];
        for (uint32_t bit = 0; bit < 8u; ++bit) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}
