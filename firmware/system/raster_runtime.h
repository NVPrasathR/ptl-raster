#pragma once

#include <stdbool.h>
#include <stdint.h>

void raster_system_init(void);
void raster_system_tick(uint32_t tick_ms);
void raster_system_set_status_led(bool enabled);
void raster_system_heartbeat(uint32_t tick_ms);
