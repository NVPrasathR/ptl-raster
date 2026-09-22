#include "w5500_eth.h"

#if defined(PICO_BUILD)
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "../board_pins.h"
#include "http_server.h"
#include "socket.h"
#include "dhcp.h"
#include "wizchip_conf.h"
#include "w5500.h"
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

#if defined(PICO_BUILD)
static bool parse_ipv4(const char *text, uint8_t result[4]) {
    unsigned int octets[4];
    char trailing;
    if (text == NULL || sscanf(text, "%u.%u.%u.%u%c", &octets[0], &octets[1],
                               &octets[2], &octets[3], &trailing) != 4) {
        return false;
    }
    for (size_t index = 0; index < 4U; ++index) {
        if (octets[index] > 255U) {
            return false;
        }
        result[index] = (uint8_t)octets[index];
    }
    return true;
}

static void format_ipv4(char output[W5500_ETH_MAX_IP_LEN], const uint8_t ip[4]) {
    (void)snprintf(output, W5500_ETH_MAX_IP_LEN, "%u.%u.%u.%u",
                   (unsigned int)ip[0], (unsigned int)ip[1],
                   (unsigned int)ip[2], (unsigned int)ip[3]);
}
#endif

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
        ctx->mac[0] = 0x02U;
        ctx->mac[1] = 0x00U;
        ctx->mac[2] = 0x00U;
        ctx->mac[3] = 0x00U;
        ctx->mac[4] = 0x00U;
        ctx->mac[5] = 0x01U;
    }

    ctx->dhcp_enabled = dhcp_enabled;
    ctx->http_port = (http_port > 0) ? http_port : 80;
    ctx->retry_interval_ms = 2000U;
    ctx->next_retry_ms = 0U;
    ctx->state = W5500_NET_STATE_INIT;
    set_string_or_default(ctx->ip, sizeof(ctx->ip), ip, "172.17.0.102");
    set_string_or_default(ctx->subnet, sizeof(ctx->subnet), subnet, "255.255.252.0");
    set_string_or_default(ctx->gateway, sizeof(ctx->gateway), gateway, "172.17.3.254");
    set_string_or_default(ctx->dns, sizeof(ctx->dns), dns, "172.17.3.254");
    set_string_or_default(ctx->device_name, sizeof(ctx->device_name), device_name,
                          "raster-pick-to-light");
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

#if defined(PICO_BUILD)
#define W5500_HTTP_SOCKET 0U
#define W5500_DHCP_SOCKET 1U
#define W5500_DATA_SOCKET 2U
#define W5500_DATA_PORT 8109U
#define W5500_DATA_BUFFER_SIZE 4096U
#define W5500_DHCP_BUFFER_SIZE 2048U

static uint8_t g_dhcp_buffer[W5500_DHCP_BUFFER_SIZE];
static char g_request[HTTP_SERVER_MAX_BODY + 512U];
static char g_response[HTTP_SERVER_RESPONSE_CAPACITY];
static char g_data_buffer[W5500_DATA_BUFFER_SIZE];
static size_t g_data_used;
static size_t g_request_used;
static bool g_dhcp_running;
static uint32_t g_last_dhcp_tick_ms;
static w5500_eth_context_t *g_active_ctx;

static void w5500_select(void) {
    gpio_put(RASTER_W5500_CS_PIN, 0);
}

static void w5500_deselect(void) {
    gpio_put(RASTER_W5500_CS_PIN, 1);
}

static uint8_t w5500_spi_read_byte(void) {
    uint8_t tx = 0xFFU;
    uint8_t rx = 0U;
    spi_write_read_blocking(spi0, &tx, &rx, 1U);
    return rx;
}

static void w5500_spi_write_byte(uint8_t data) {
    (void)spi_write_blocking(spi0, &data, 1U);
}

static void w5500_spi_read_burst(uint8_t *buffer, uint16_t length) {
    spi_read_blocking(spi0, 0xFFU, buffer, length);
}

static void w5500_spi_write_burst(uint8_t *buffer, uint16_t length) {
    (void)spi_write_blocking(spi0, buffer, length);
}

static void w5500_gpio_spi_init(void) {
    spi_init(spi0, 10000000U);
    gpio_set_function(RASTER_W5500_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(RASTER_W5500_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(RASTER_W5500_SCLK_PIN, GPIO_FUNC_SPI);

    gpio_init(RASTER_W5500_CS_PIN);
    gpio_set_dir(RASTER_W5500_CS_PIN, GPIO_OUT);
    gpio_put(RASTER_W5500_CS_PIN, 1);

    gpio_init(RASTER_W5500_RESET_PIN);
    gpio_set_dir(RASTER_W5500_RESET_PIN, GPIO_OUT);
    gpio_put(RASTER_W5500_RESET_PIN, 1);

    gpio_init(RASTER_W5500_INTERRUPT_PIN);
    gpio_set_dir(RASTER_W5500_INTERRUPT_PIN, GPIO_IN);
    gpio_pull_up(RASTER_W5500_INTERRUPT_PIN);
}

static void w5500_reset_chip(void) {
    gpio_put(RASTER_W5500_RESET_PIN, 0);
    sleep_ms(100U);
    gpio_put(RASTER_W5500_RESET_PIN, 1);
    sleep_ms(500U);
}

static void w5500_update_ctx_from_netinfo(w5500_eth_context_t *ctx) {
    wiz_NetInfo netinfo;
    if (ctx == NULL) {
        return;
    }
    ctlnetwork(CN_GET_NETINFO, &netinfo);
    format_ipv4(ctx->ip, netinfo.ip);
    format_ipv4(ctx->subnet, netinfo.sn);
    format_ipv4(ctx->gateway, netinfo.gw);
    format_ipv4(ctx->dns, netinfo.dns);
}

static bool w5500_apply_static_network(const w5500_eth_context_t *ctx) {
    wiz_NetInfo netinfo;
    if (ctx == NULL ||
        !parse_ipv4(ctx->ip, netinfo.ip) ||
        !parse_ipv4(ctx->subnet, netinfo.sn) ||
        !parse_ipv4(ctx->gateway, netinfo.gw) ||
        !parse_ipv4(ctx->dns, netinfo.dns)) {
        return false;
    }
    memcpy(netinfo.mac, ctx->mac, sizeof(ctx->mac));
    netinfo.dhcp = NETINFO_STATIC;
    ctlnetwork(CN_SET_NETINFO, &netinfo);
    return true;
}

static bool w5500_start_dhcp(w5500_eth_context_t *ctx) {
    wiz_NetInfo netinfo;
    if (ctx == NULL) {
        return false;
    }
    memset(&netinfo, 0, sizeof(netinfo));
    memcpy(netinfo.mac, ctx->mac, sizeof(ctx->mac));
    netinfo.dhcp = NETINFO_DHCP;
    ctlnetwork(CN_SET_NETINFO, &netinfo);
    DHCP_init(W5500_DHCP_SOCKET, g_dhcp_buffer);
    g_last_dhcp_tick_ms = 0U;
    g_dhcp_running = true;
    return true;
}

static void w5500_dhcp_assigned(void) {
    if (g_active_ctx != NULL) {
        w5500_update_ctx_from_netinfo(g_active_ctx);
        g_active_ctx->state = W5500_NET_STATE_DHCP_READY;
        g_active_ctx->socket_ready = true;
    }
}

static void w5500_dhcp_changed(void) {
    w5500_dhcp_assigned();
}

static void w5500_dhcp_conflict(void) {
    if (g_active_ctx != NULL) {
        g_active_ctx->socket_ready = false;
        g_active_ctx->state = W5500_NET_STATE_RECOVERY;
        g_active_ctx->recovery_active = true;
    }
    g_dhcp_running = false;
}

static bool w5500_send_http_response(const char *response, size_t response_length) {
    static const size_t max_chunk_size = 1024U;
    size_t offset = 0U;
    if (response == NULL) {
        return false;
    }
    while (offset < response_length) {
        size_t remaining = response_length - offset;
        uint16_t request_length =
            (remaining > max_chunk_size) ? (uint16_t)max_chunk_size : (uint16_t)remaining;
        int32_t written = send(W5500_HTTP_SOCKET,
                               (uint8_t *)response + offset,
                               request_length);
        if (written == SOCK_BUSY) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        offset += (size_t)written;
    }
    return true;
}

static bool w5500_wait_for_http_tx_complete(void) {
    absolute_time_t timeout = make_timeout_time_ms(2000);
    while (!time_reached(timeout)) {
        uint8_t state = getSn_SR(W5500_HTTP_SOCKET);
        if (state != SOCK_ESTABLISHED && state != SOCK_CLOSE_WAIT) {
            return false;
        }
        if (getSn_IR(W5500_HTTP_SOCKET) & Sn_IR_TIMEOUT) {
            setSn_IR(W5500_HTTP_SOCKET, Sn_IR_TIMEOUT);
            return false;
        }
        if (getSn_TX_FSR(W5500_HTTP_SOCKET) == getSn_TxMAX(W5500_HTTP_SOCKET)) {
            if (getSn_IR(W5500_HTTP_SOCKET) & Sn_IR_SENDOK) {
                setSn_IR(W5500_HTTP_SOCKET, Sn_IR_SENDOK);
            }
            return true;
        }
        sleep_ms(1);
    }
    return false;
}
#endif

void w5500_eth_update(w5500_eth_context_t *ctx, uint32_t now_ms, bool link_detected) {
    if (ctx == NULL || !ctx->initialized) {
        return;
    }

    if (!link_detected) {
        ctx->link_up = false;
        ctx->socket_ready = false;
#if defined(PICO_BUILD)
        if (g_dhcp_running) {
            DHCP_stop();
            g_dhcp_running = false;
        }
        (void)disconnect(W5500_HTTP_SOCKET);
        (void)close(W5500_HTTP_SOCKET);
        (void)close(W5500_DHCP_SOCKET);
        g_request_used = 0U;
#endif
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
            if (ctx->dhcp_enabled) {
#if defined(PICO_BUILD)
                if (!w5500_start_dhcp(ctx)) {
                    ctx->state = W5500_NET_STATE_RECOVERY;
                    ctx->recovery_active = true;
                    ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
                    break;
                }
                ctx->socket_ready = false;
#else
                ctx->socket_ready = true;
#endif
                ctx->state = W5500_NET_STATE_DHCP_WAIT;
            } else {
#if defined(PICO_BUILD)
                if (!w5500_apply_static_network(ctx)) {
                    ctx->state = W5500_NET_STATE_RECOVERY;
                    ctx->recovery_active = true;
                    ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
                    break;
                }
#endif
                ctx->state = W5500_NET_STATE_STATIC_READY;
                ctx->socket_ready = true;
            }
            break;
        case W5500_NET_STATE_DHCP_WAIT:
#if defined(PICO_BUILD)
            if (!g_dhcp_running) {
                if (!w5500_start_dhcp(ctx)) {
                    ctx->state = W5500_NET_STATE_RECOVERY;
                    ctx->recovery_active = true;
                    ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
                    break;
                }
            }
            if (g_last_dhcp_tick_ms == 0U) {
                g_last_dhcp_tick_ms = now_ms;
            }
            while ((uint32_t)(now_ms - g_last_dhcp_tick_ms) >= 1000U) {
                DHCP_time_handler();
                g_last_dhcp_tick_ms += 1000U;
            }
            switch (DHCP_run()) {
                case DHCP_IP_ASSIGN:
                case DHCP_IP_CHANGED:
                case DHCP_IP_LEASED:
                    w5500_update_ctx_from_netinfo(ctx);
                    ctx->state = W5500_NET_STATE_DHCP_READY;
                    ctx->socket_ready = true;
                    break;
                case DHCP_FAILED:
                    ctx->socket_ready = false;
                    ctx->state = W5500_NET_STATE_RECOVERY;
                    ctx->recovery_active = true;
                    ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
                    g_dhcp_running = false;
                    break;
                default:
                    break;
            }
#else
            ctx->socket_ready = true;
#endif
            break;
        case W5500_NET_STATE_STATIC_READY:
            ctx->socket_ready = true;
            break;
        case W5500_NET_STATE_DHCP_READY:
            ctx->socket_ready = true;
#if defined(PICO_BUILD)
            if (g_dhcp_running) {
                while ((uint32_t)(now_ms - g_last_dhcp_tick_ms) >= 1000U) {
                    DHCP_time_handler();
                    g_last_dhcp_tick_ms += 1000U;
                }
                switch (DHCP_run()) {
                    case DHCP_IP_ASSIGN:
                    case DHCP_IP_CHANGED:
                    case DHCP_IP_LEASED:
                        w5500_update_ctx_from_netinfo(ctx);
                        break;
                    case DHCP_FAILED:
                        ctx->socket_ready = false;
                        ctx->state = W5500_NET_STATE_RECOVERY;
                        ctx->recovery_active = true;
                        ctx->next_retry_ms = now_ms + ctx->retry_interval_ms;
                        g_dhcp_running = false;
                        break;
                    default:
                        break;
                }
            }
#endif
            break;
        case W5500_NET_STATE_RECOVERY:
            if (now_ms >= ctx->next_retry_ms) {
                ctx->recovery_active = false;
                ctx->state = ctx->dhcp_enabled ? W5500_NET_STATE_DHCP_WAIT
                                               : W5500_NET_STATE_STATIC_READY;
#if defined(PICO_BUILD)
                if (ctx->dhcp_enabled) {
                    if (w5500_start_dhcp(ctx)) {
                        ctx->socket_ready = false;
                    }
                } else if (w5500_apply_static_network(ctx)) {
                    ctx->socket_ready = true;
                }
#endif
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
    return ctx != NULL && ctx->dhcp_enabled &&
           (ctx->state == W5500_NET_STATE_DHCP_WAIT ||
            ctx->state == W5500_NET_STATE_DHCP_READY);
}

const char *w5500_eth_state_name(w5500_eth_context_t *ctx) {
    if (ctx == NULL) {
        return "invalid";
    }
    switch (ctx->state) {
        case W5500_NET_STATE_INIT:
            return "init";
        case W5500_NET_STATE_LINK_WAIT:
            return "link_wait";
        case W5500_NET_STATE_DHCP_WAIT:
            return "dhcp_wait";
        case W5500_NET_STATE_DHCP_READY:
            return "dhcp_ready";
        case W5500_NET_STATE_STATIC_READY:
            return "static_ready";
        case W5500_NET_STATE_RECOVERY:
            return "recovery";
        default:
            return "unknown";
    }
}

#if defined(PICO_BUILD)
bool w5500_hw_init(w5500_eth_context_t *ctx) {
    uint8_t mem_size[8] = {2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U};

    if (ctx == NULL) {
        return false;
    }

    w5500_gpio_spi_init();
    w5500_reset_chip();

    reg_wizchip_cs_cbfunc(w5500_select, w5500_deselect);
    reg_wizchip_spi_cbfunc(w5500_spi_read_byte, w5500_spi_write_byte);
    reg_wizchip_spiburst_cbfunc(w5500_spi_read_burst, w5500_spi_write_burst);

    if (ctlwizchip(CW_INIT_WIZCHIP, mem_size) == -1) {
        return false;
    }
    setSHAR(ctx->mac);
    if (getVERSIONR() != 0x04U) {
        return false;
    }

    g_active_ctx = ctx;
    g_dhcp_running = false;
    g_last_dhcp_tick_ms = 0U;
    g_request_used = 0U;
    reg_dhcp_cbfunc(w5500_dhcp_assigned, w5500_dhcp_changed, w5500_dhcp_conflict);

    if (!ctx->dhcp_enabled && !w5500_apply_static_network(ctx)) {
        return false;
    }
    if (!ctx->dhcp_enabled) {
        ctx->state = W5500_NET_STATE_STATIC_READY;
        ctx->socket_ready = true;
    } else {
        ctx->state = W5500_NET_STATE_INIT;
        ctx->socket_ready = false;
    }
    return true;
}

bool w5500_hw_link_up(void) {
    return (getPHYCFGR() & PHYCFGR_LNK_ON) == PHYCFGR_LNK_ON;
}

void w5500_http_service(w5500_eth_context_t *ctx, w5500_http_handler_fn handler,
                        void *handler_context) {
    uint8_t state;
    uint16_t available;
    int32_t length;

    if (ctx == NULL || handler == NULL || !w5500_eth_is_ready(ctx) ||
        (ctx->dhcp_enabled && ctx->state != W5500_NET_STATE_DHCP_READY &&
         ctx->state != W5500_NET_STATE_STATIC_READY)) {
        return;
    }

    state = getSn_SR(W5500_HTTP_SOCKET);
    switch (state) {
        case SOCK_CLOSED:
            g_request_used = 0U;
            if (socket(W5500_HTTP_SOCKET, Sn_MR_TCP, (uint16_t)ctx->http_port, 0U) ==
                W5500_HTTP_SOCKET) {
                (void)listen(W5500_HTTP_SOCKET);
            }
            return;
        case SOCK_INIT:
            (void)listen(W5500_HTTP_SOCKET);
            return;
        case SOCK_CLOSE_WAIT:
            (void)close(W5500_HTTP_SOCKET);
            g_request_used = 0U;
            return;
        case SOCK_ESTABLISHED:
            break;
        default:
            (void)close(W5500_HTTP_SOCKET);
            g_request_used = 0U;
            return;
    }

    if ((getSn_IR(W5500_HTTP_SOCKET) & Sn_IR_CON) != 0U) {
        setSn_IR(W5500_HTTP_SOCKET, Sn_IR_CON);
    }

    available = getSn_RX_RSR(W5500_HTTP_SOCKET);
    if (available == 0U) {
        return;
    }
    if ((size_t)available >= sizeof(g_request) - g_request_used) {
        g_request_used = 0U;
        (void)disconnect(W5500_HTTP_SOCKET);
        return;
    }

    length = recv(W5500_HTTP_SOCKET, (uint8_t *)g_request + g_request_used, available);
    if (length <= 0) {
        return;
    }
    g_request_used += (size_t)length;
    g_request[g_request_used] = '\0';

    {
        char *header_end = strstr(g_request, "\r\n\r\n");
        size_t header_length;
        size_t content_length = 0U;
        const char *content;
        int response_length;

        if (header_end == NULL) {
            return;
        }
        header_length = (size_t)(header_end - g_request) + 4U;
        content = g_request;
        while (content < header_end) {
            const char *line_end = strstr(content, "\r\n");
            static const char name[] = "Content-Length:";
            if (line_end == NULL || line_end > header_end) {
                break;
            }
            if ((size_t)(line_end - content) >= sizeof(name) - 1U &&
                strncasecmp(content, name, sizeof(name) - 1U) == 0) {
                char *end = NULL;
                unsigned long parsed = strtoul(content + sizeof(name) - 1U, &end, 10);
                if (end == content + sizeof(name) - 1U || parsed > HTTP_SERVER_MAX_BODY) {
                    g_request_used = 0U;
                    (void)disconnect(W5500_HTTP_SOCKET);
                    return;
                }
                content_length = (size_t)parsed;
            }
            content = line_end + 2U;
        }
        if (g_request_used < header_length + content_length) {
            return;
        }
        if (g_request_used != header_length + content_length) {
            g_request_used = 0U;
            (void)disconnect(W5500_HTTP_SOCKET);
            return;
        }

        response_length = handler(g_request, g_request_used, g_response, sizeof(g_response),
                                  handler_context);
        if (response_length > 0) {
            if (!w5500_send_http_response(g_response, (size_t)response_length) ||
                !w5500_wait_for_http_tx_complete()) {
                (void)close(W5500_HTTP_SOCKET);
                g_request_used = 0U;
                return;
            }
        }
    }
    (void)close(W5500_HTTP_SOCKET);
    g_request_used = 0U;
}

void w5500_data_server_service(w5500_eth_context_t *ctx,
                               w5500_data_handler_fn handler,
                               void *handler_context) {
    uint8_t state;
    uint16_t available;
    int32_t length;
    size_t payload_start = 0U;
    size_t payload_end = 0U;

    if (ctx == NULL || handler == NULL || !w5500_eth_is_ready(ctx)) {
        return;
    }

    state = getSn_SR(W5500_DATA_SOCKET);
    switch (state) {
        case SOCK_CLOSED:
            g_data_used = 0U;
            if (socket(W5500_DATA_SOCKET, Sn_MR_TCP, W5500_DATA_PORT, 0U) ==
                W5500_DATA_SOCKET) {
                (void)listen(W5500_DATA_SOCKET);
            }
            return;
        case SOCK_INIT:
            (void)listen(W5500_DATA_SOCKET);
            return;
        case SOCK_CLOSE_WAIT:
            (void)close(W5500_DATA_SOCKET);
            g_data_used = 0U;
            return;
        case SOCK_ESTABLISHED:
            break;
        default:
            (void)close(W5500_DATA_SOCKET);
            g_data_used = 0U;
            return;
    }

    if ((getSn_IR(W5500_DATA_SOCKET) & Sn_IR_CON) != 0U) {
        setSn_IR(W5500_DATA_SOCKET, Sn_IR_CON);
    }
    available = getSn_RX_RSR(W5500_DATA_SOCKET);
    if (available == 0U) {
        return;
    }
    if ((size_t)available >= sizeof(g_data_buffer) - g_data_used) {
        g_data_used = 0U;
        (void)disconnect(W5500_DATA_SOCKET);
        return;
    }

    length = recv(W5500_DATA_SOCKET, (uint8_t *)g_data_buffer + g_data_used,
                  available);
    if (length <= 0) {
        return;
    }
    g_data_used += (size_t)length;
    g_data_buffer[g_data_used] = '\0';

    while (payload_start < g_data_used &&
           (g_data_buffer[payload_start] == ' ' ||
            g_data_buffer[payload_start] == '\t' ||
            g_data_buffer[payload_start] == '\r' ||
            g_data_buffer[payload_start] == '\n')) {
        ++payload_start;
    }
    payload_end = g_data_used;
    while (payload_end > payload_start &&
           (g_data_buffer[payload_end - 1U] == ' ' ||
            g_data_buffer[payload_end - 1U] == '\t' ||
            g_data_buffer[payload_end - 1U] == '\r' ||
            g_data_buffer[payload_end - 1U] == '\n')) {
        --payload_end;
    }
    if (payload_end > payload_start && g_data_buffer[payload_start] == '[' &&
        g_data_buffer[payload_end - 1U] == ']') {
        (void)handler(g_data_buffer + payload_start, payload_end - payload_start,
                       handler_context);
        g_data_used = 0U;
        (void)disconnect(W5500_DATA_SOCKET);
    }
}
#endif
