#pragma once

#include <stdbool.h>
#include <stdint.h>

#define RASTER_FIRMWARE_NAME "raster-pick-to-light"
#define RASTER_SDK_PLATFORM "RP2350/RP2354 family"
#define RASTER_PACKAGE_FAMILY "RP2350B/RP2354B package assumption (48 GPIO QFN-80)"
#define RASTER_BOARD_IS_CUSTOM 1u

/*
 * Important scope note:
 *   - RP235x silicon is supported by Pico SDK as rp2350 family.
 *   - RP2354B is a package/marketing variant of the same family; package pinout and
 *     flash details must be confirmed against the actual schematic rather than assumed.
 *   - This project keeps board-level flash, LED chain, and Ethernet pin assignments in
 *     board_pins.h and documents them as draft only unless a schematic proves them.
 */
#define RASTER_HAS_INTERNAL_FLASH 0u
#define RASTER_HAS_EXTERNAL_FLASH 1u
#define RASTER_HAS_ETHERNET 1u
#define RASTER_HAS_OLED 1u
#define RASTER_HAS_BUZZER 1u
#define RASTER_HAS_USER_BUTTON 1u
#define RASTER_HAS_STATUS_LED 1u

#define RASTER_TASK_TICK_MS 10u
#define RASTER_HEARTBEAT_MS 500u
