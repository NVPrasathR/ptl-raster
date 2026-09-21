#ifndef W5500_ETH_H
#define W5500_ETH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W5500_ETH_MAX_IP_LEN 16
#define W5500_ETH_MAX_HOST_LEN 32

typedef enum {
    W5500_NET_STATE_INIT = 0,
    W5500_NET_STATE_LINK_WAIT,
    W5500_NET_STATE_DHCP_WAIT,
    W5500_NET_STATE_DHCP_READY,
    W5500_NET_STATE_STATIC_READY,
    W5500_NET_STATE_RECOVERY
} w5500_net_state_t;

typedef struct {
    bool initialized;
    bool dhcp_enabled;
    bool link_up;
    bool socket_ready;
    bool recovery_active;
    uint32_t retry_interval_ms;
    uint32_t next_retry_ms;
    uint8_t mac[6];
    char ip[W5500_ETH_MAX_IP_LEN];
    char subnet[W5500_ETH_MAX_IP_LEN];
    char gateway[W5500_ETH_MAX_IP_LEN];
    char dns[W5500_ETH_MAX_IP_LEN];
    char device_name[W5500_ETH_MAX_HOST_LEN];
    int http_port;
    w5500_net_state_t state;
} w5500_eth_context_t;

void w5500_eth_init(w5500_eth_context_t *ctx,
                   const uint8_t mac[6],
                   bool dhcp_enabled,
                   const char *ip,
                   const char *subnet,
                   const char *gateway,
                   const char *dns,
                   const char *device_name,
                   int http_port);

void w5500_eth_update(w5500_eth_context_t *ctx, uint32_t now_ms, bool link_detected);
void w5500_eth_set_static_config(w5500_eth_context_t *ctx,
                                const char *ip,
                                const char *subnet,
                                const char *gateway,
                                const char *dns,
                                int http_port,
                                bool dhcp_enabled);

bool w5500_eth_is_ready(const w5500_eth_context_t *ctx);
bool w5500_eth_link_is_up(const w5500_eth_context_t *ctx);
bool w5500_eth_is_dhcp_active(const w5500_eth_context_t *ctx);
const char *w5500_eth_state_name(w5500_eth_context_t *ctx);

#if defined(PICO_BUILD)
typedef int (*w5500_http_handler_fn)(const char *request, size_t request_length,
                                     char *response, size_t response_capacity,
                                     void *context);
bool w5500_hw_init(w5500_eth_context_t *ctx);
bool w5500_hw_link_up(void);
void w5500_http_service(w5500_eth_context_t *ctx, w5500_http_handler_fn handler,
                        void *handler_context);
#endif

#ifdef __cplusplus
}
#endif

#endif
