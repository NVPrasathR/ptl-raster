#pragma once

#include <stdint.h>

#include "ptl_result.h"

typedef enum { OLED_NETWORK_UNKNOWN, OLED_NETWORK_DHCP, OLED_NETWORK_CONNECTED,
               OLED_NETWORK_DISCONNECTED } oled_network_state_t;
typedef enum { OLED_MESSAGE_NONE, OLED_MESSAGE_CONFIG_SAVED, OLED_MESSAGE_OTA_UPDATE,
               OLED_MESSAGE_OTA_SUCCESS, OLED_MESSAGE_OTA_FAILED, OLED_MESSAGE_FACTORY_RESET,
               OLED_MESSAGE_NETWORK_ERROR, OLED_MESSAGE_SYSTEM_ERROR } oled_message_t;

ptl_result_t oled_manager_init(void);
void oled_manager_deinit(void);
void oled_manager_service(void);
void oled_manager_set_network_state(oled_network_state_t state);
void oled_manager_set_network_diagnostic(uint8_t state, uint32_t error_code);
void oled_manager_set_network_detail(const char *detail);
void oled_manager_set_ip(uint32_t ip_address);
void oled_manager_set_fw_version(const char *version);
void oled_manager_show_message(oled_message_t message);
void oled_manager_show_ota_progress(uint8_t progress_percent);
void oled_manager_show_error(const char *message);
