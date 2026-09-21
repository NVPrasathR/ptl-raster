#include "w5500_eth.h"

#if defined(PICO_BUILD)
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "../board_pins.h"
#include "http_server.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void set_string_or_default(char *dst, size_t cap, const char *value, const char *fallback) {
    if (dst == NULL || cap == 0U) {
        return;
    }
    if (value != NULL && value[0] != '\0') {
        strncpy(dst, value, cap - 1U);
        dst[cap - 1U] = '\0';
    } else if (fallback != dst) {
        strncpy(dst, fallback, cap - 1U);
        dst[cap - 1U] = '\0';
    }
}

void w5500_eth_init(w5500_eth_context_t *ctx,
                   const uint8_t mac[6],
                   bool dhcp_enabled,
                   const char *ip,
                   const char *subnet,
                   const char *gateway,
                   const char *dns,
                   const char *device_name,
                   int http_port) {
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    if (mac != NULL) {
        memcpy(ctx->mac, mac, sizeof(ctx->mac));
    } else {
        ctx->mac[0] = 0x02; ctx->mac[1] = 0x00; ctx->mac[2] = 0x00;
        ctx->mac[3] = 0x00; ctx->mac[4] = 0x00; ctx->mac[5] = 0x01;
    }

    ctx->dhcp_enabled = dhcp_enabled;
    ctx->http_port = (http_port > 0) ? http_port : 80;
    ctx->retry_interval_ms = 2000U;
    ctx->next_retry_ms = 0U;
    ctx->state = W5500_NET_STATE_INIT;
    set_string_or_default(ctx->ip, sizeof(ctx->ip), ip, "192.168.1.10");
    set_string_or_default(ctx->subnet, sizeof(ctx->subnet), subnet, "255.255.255.0");
    set_string_or_default(ctx->gateway, sizeof(ctx->gateway), gateway, "192.168.1.1");
    set_string_or_default(ctx->dns, sizeof(ctx->dns), dns, "1.1.1.1");
    set_string_or_default(ctx->device_name, sizeof(ctx->device_name), device_name, "raster-pick-to-light");
    ctx->initialized = true;
}

void w5500_eth_set_static_config(w5500_eth_context_t *ctx,
                                const char *ip,
                                const char *subnet,
                                const char *gateway,
                                const char *dns,
                                int http_port,
                                bool dhcp_enabled) {
    if (ctx == NULL) {
        return;
    }
    ctx->dhcp_enabled = dhcp_enabled;
    if (http_port > 0) {
        ctx->http_port = http_port;
    }
    if (ip != NULL) {
        set_string_or_default(ctx->ip, sizeof(ctx->ip), ip, ctx->ip);
    }
    if (subnet != NULL) {
        set_string_or_default(ctx->subnet, sizeof(ctx->subnet), subnet, ctx->subnet);
    }
    if (gateway != NULL) {
        set_string_or_default(ctx->gateway, sizeof(ctx->gateway), gateway, ctx->gateway);
    }
    if (dns != NULL) {
        set_string_or_default(ctx->dns, sizeof(ctx->dns), dns, ctx->dns);
    }
}

void w5500_eth_update(w5500_eth_context_t *ctx, uint32_t now_ms, bool link_detected) {
    if (ctx == NULL || !ctx->initialized) {
        return;
    }

    if (!link_detected) {
        ctx->link_up = false;
        ctx->socket_ready = false;
        if (ctx->state != W5500_NET_STATE_RECOVERY) {
            ctx->state = W5500_NET_STATE_RECOVERY;
            ctx->recovery_active = true;
            ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
        }
        return;
    }

    ctx->link_up = true;
    switch (ctx->state) {
        case W5500_NET_STATE_INIT:
        case W5500_NET_STATE_LINK_WAIT:
            ctx->state = ctx->dhcp_enabled ? W5500_NET_STATE_DHCP_WAIT : W5500_NET_STATE_STATIC_READY;
            ctx->socket_ready = true;
            break;
        case W5500_NET_STATE_DHCP_WAIT:
            ctx->socket_ready = true;
            break;
        case W5500_NET_STATE_STATIC_READY:
            ctx->socket_ready = true;
            break;
        case W5500_NET_STATE_DHCP_READY:
            ctx->socket_ready = true;
            break;
        case W5500_NET_STATE_RECOVERY:
            if (now_ms >= ctx->next_retry_ms) {
                ctx->recovery_active = false;
                ctx->state = ctx->dhcp_enabled ? W5500_NET_STATE_DHCP_WAIT : W5500_NET_STATE_STATIC_READY;
            }
            break;
        default:
            break;
    }
}

bool w5500_eth_is_ready(const w5500_eth_context_t *ctx) {
    return ctx != NULL && ctx->initialized && ctx->link_up && ctx->socket_ready;
}

bool w5500_eth_link_is_up(const w5500_eth_context_t *ctx) {
    return ctx != NULL && ctx->link_up;
}

bool w5500_eth_is_dhcp_active(const w5500_eth_context_t *ctx) {
    return ctx != NULL && ctx->dhcp_enabled && (ctx->state == W5500_NET_STATE_DHCP_WAIT || ctx->state == W5500_NET_STATE_DHCP_READY);
}

const char *w5500_eth_state_name(w5500_eth_context_t *ctx) {
    if (ctx == NULL) {
        return "invalid";
    }
    switch (ctx->state) {
        case W5500_NET_STATE_INIT: return "init";
        case W5500_NET_STATE_LINK_WAIT: return "link_wait";
        case W5500_NET_STATE_DHCP_WAIT: return "dhcp_wait";
        case W5500_NET_STATE_DHCP_READY: return "dhcp_ready";
        case W5500_NET_STATE_STATIC_READY: return "static_ready";
        case W5500_NET_STATE_RECOVERY: return "recovery";
        default: return "unknown";
    }
}

#if defined(PICO_BUILD)
#define W5500_COMMON 0u
#define W5500_S0_REG 1u
#define W5500_S0_TX 2u
#define W5500_S0_RX 3u
#define W5500_SOCKET_BUFFER_MASK 0x07ffu

    static void w5500_select(void) { gpio_put(RASTER_W5500_CS_PIN, 0); }
    static void w5500_deselect(void) { gpio_put(RASTER_W5500_CS_PIN, 1); }

    static void w5500_transfer(uint16_t address, uint8_t block, bool write,
                               const uint8_t *tx, uint8_t *rx, size_t length) {
        uint8_t header[3] = {(uint8_t)(address >> 8), (uint8_t)address,
                             (uint8_t)((block << 3) | (write ? 0x04u : 0u))};
        w5500_select();
        spi_write_blocking(spi0, header, sizeof(header));
        if (write) {
            spi_write_blocking(spi0, tx, length);
        } else {
            spi_read_blocking(spi0, 0u, rx, length);
        }
        w5500_deselect();
    }

    static uint8_t w5500_read8(uint16_t address, uint8_t block) {
        uint8_t value;
        w5500_transfer(address, block, false, NULL, &value, 1u);
        return value;
    }

    static uint16_t w5500_read16(uint16_t address, uint8_t block) {
        uint8_t bytes[2];
        w5500_transfer(address, block, false, NULL, bytes, sizeof(bytes));
        return (uint16_t)((uint16_t)bytes[0] << 8) | bytes[1];
    }

    static void w5500_write8(uint16_t address, uint8_t block, uint8_t value) {
        w5500_transfer(address, block, true, &value, NULL, 1u);
    }

    static void w5500_write16(uint16_t address, uint8_t block, uint16_t value) {
        uint8_t bytes[2] = {(uint8_t)(value >> 8), (uint8_t)value};
        w5500_transfer(address, block, true, bytes, NULL, sizeof(bytes));
    }

    static void w5500_write_bytes(uint16_t address, uint8_t block,
                                  const uint8_t *bytes, size_t length) {
        w5500_transfer(address, block, true, bytes, NULL, length);
    }

    static bool parse_ipv4(const char *text, uint8_t result[4]) {
        unsigned int octets[4];
        char trailing;
        if (text == NULL || sscanf(text, "%u.%u.%u.%u%c", &octets[0], &octets[1],
                                   &octets[2], &octets[3], &trailing) != 4) {
            return false;
        }
        for (size_t index = 0; index < 4u; ++index) {
            if (octets[index] > 255u) return false;
            result[index] = (uint8_t)octets[index];
        }
        return true;
    }

    static bool wait_socket_command(void) {
        for (uint32_t retry = 0; retry < 10000u; ++retry) {
            if (w5500_read8(0x0001u, W5500_S0_REG) == 0u) return true;
        }
        return false;
    }

    static bool socket_command(uint8_t command) {
        w5500_write8(0x0001u, W5500_S0_REG, command);
        return wait_socket_command();
    }

    static void socket_open_listen(uint16_t port) {
        (void)socket_command(0x10u);
        w5500_write8(0x0000u, W5500_S0_REG, 0x01u);
        w5500_write16(0x0004u, W5500_S0_REG, port);
        if (socket_command(0x01u)) (void)socket_command(0x02u);
    }

    bool w5500_hw_init(w5500_eth_context_t *ctx) {
        uint8_t ip[4], subnet[4], gateway[4];
        if (ctx == NULL ||
            !parse_ipv4(ctx->ip, ip) || !parse_ipv4(ctx->subnet, subnet) ||
            !parse_ipv4(ctx->gateway, gateway)) {
            return false;
        }
        spi_init(spi0, 12000000u);
        gpio_set_function(RASTER_W5500_MISO_PIN, GPIO_FUNC_SPI);
        gpio_set_function(RASTER_W5500_MOSI_PIN, GPIO_FUNC_SPI);
        gpio_set_function(RASTER_W5500_SCLK_PIN, GPIO_FUNC_SPI);
        gpio_init(RASTER_W5500_CS_PIN);
        gpio_set_dir(RASTER_W5500_CS_PIN, GPIO_OUT);
        w5500_deselect();
        gpio_init(RASTER_W5500_RESET_PIN);
        gpio_set_dir(RASTER_W5500_RESET_PIN, GPIO_OUT);
        gpio_put(RASTER_W5500_RESET_PIN, 0);
        sleep_ms(2u);
        gpio_put(RASTER_W5500_RESET_PIN, 1);
        sleep_ms(50u);
        w5500_write_bytes(0x0001u, W5500_COMMON, gateway, sizeof(gateway));
        w5500_write_bytes(0x0005u, W5500_COMMON, subnet, sizeof(subnet));
        w5500_write_bytes(0x0009u, W5500_COMMON, ctx->mac, sizeof(ctx->mac));
        w5500_write_bytes(0x000fu, W5500_COMMON, ip, sizeof(ip));
        socket_open_listen((uint16_t)ctx->http_port);
        return true;
    }

    bool w5500_hw_link_up(void) {
        return (w5500_read8(0x002eu, W5500_COMMON) & 0x01u) != 0u;
    }

    static bool socket_send(const uint8_t *data, size_t length) {
        while (length != 0u) {
            uint16_t chunk = length > 2048u ? 2048u : (uint16_t)length;
            uint16_t free_size = 0u;
            for (uint32_t retry = 0; retry < 100000u && free_size < chunk; ++retry) {
                free_size = w5500_read16(0x0020u, W5500_S0_REG);
            }
            if (free_size < chunk) return false;
            uint16_t pointer = w5500_read16(0x0024u, W5500_S0_REG);
            uint16_t offset = pointer & W5500_SOCKET_BUFFER_MASK;
            uint16_t first = (uint16_t)(2048u - offset);
            if (first > chunk) first = chunk;
            w5500_write_bytes(offset, W5500_S0_TX, data, first);
            if (first < chunk) {
                w5500_write_bytes(0u, W5500_S0_TX, data + first, chunk - first);
            }
            w5500_write16(0x0024u, W5500_S0_REG, (uint16_t)(pointer + chunk));
            if (!socket_command(0x20u)) return false;
            data += chunk;
            length -= chunk;
        }
        return true;
    }

    void w5500_http_service(w5500_eth_context_t *ctx, w5500_http_handler_fn handler,
                            void *handler_context) {
        static char request[HTTP_SERVER_MAX_BODY + 512u];
        static char response[32768u];
        static size_t request_used;
        uint8_t state;
        if (ctx == NULL || handler == NULL) return;
        state = w5500_read8(0x0003u, W5500_S0_REG);
        if (state == 0x00u || state == 0x1cu) {
            request_used = 0u;
            socket_open_listen((uint16_t)ctx->http_port);
            return;
        }
        if (state != 0x17u) {
            request_used = 0u;
            return;
        }
        uint16_t available = w5500_read16(0x0026u, W5500_S0_REG);
        if (available == 0u) return;
        if ((size_t)available >= sizeof(request) - request_used) {
            request_used = 0u;
            (void)socket_command(0x10u);
            return;
        }
        uint16_t pointer = w5500_read16(0x0028u, W5500_S0_REG);
        uint16_t offset = pointer & W5500_SOCKET_BUFFER_MASK;
        uint16_t first = (uint16_t)(2048u - offset);
        if (first > available) first = available;
        w5500_transfer(offset, W5500_S0_RX, false, NULL,
                       (uint8_t *)request + request_used, first);
        if (first < available) {
            w5500_transfer(0u, W5500_S0_RX, false, NULL,
                           (uint8_t *)request + request_used + first, available - first);
        }
        request_used += available;
        request[request_used] = '\0';
        w5500_write16(0x0028u, W5500_S0_REG, (uint16_t)(pointer + available));
        (void)socket_command(0x40u);
        char *header_end = strstr(request, "\r\n\r\n");
        if (header_end == NULL) return;
        size_t header_length = (size_t)(header_end - request) + 4u;
        size_t content_length = 0u;
        const char *content = request;
        while (content < header_end) {
            const char *line_end = strstr(content, "\r\n");
            if (line_end == NULL || line_end > header_end) break;
            static const char name[] = "Content-Length:";
            if ((size_t)(line_end - content) >= sizeof(name) - 1u &&
                strncasecmp(content, name, sizeof(name) - 1u) == 0) {
                char *end = NULL;
                unsigned long parsed = strtoul(content + sizeof(name) - 1u, &end, 10);
                if (end == content + sizeof(name) - 1u ||
                    parsed > HTTP_SERVER_MAX_BODY) {
                    request_used = 0u;
                    (void)socket_command(0x10u);
                    return;
                }
                content_length = (size_t)parsed;
            }
            content = line_end + 2u;
        }
        if (request_used < header_length + content_length) return;
        if (request_used != header_length + content_length) {
            request_used = 0u;
            (void)socket_command(0x10u);
            return;
        }
        int response_length = handler(request, request_used, response, sizeof(response),
                                      handler_context);
        if (response_length > 0) {
            (void)socket_send((const uint8_t *)response, (size_t)response_length);
        }
        (void)socket_command(0x08u);
        (void)socket_command(0x10u);
        request_used = 0u;
    }
#endif
