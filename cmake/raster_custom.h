/*
 * Custom RP2350/RP2354-compatible board definition used for local build validation.
 *
 * The RP235x family has both A (30 GPIO) and B (48 GPIO) package variants. This
 * custom board is intentionally modeled as a B-package design because GPIO 30/31/32
 * are only supported on the 48 GPIO package, and the underlying application logic
 * depends on those signals for a custom carrier board.
 *
 * This board file intentionally does not claim to be an official Raspberry Pi Pico
 * design or final schematic; it isolates the unverifiable custom-board details from
 * the silicon family assumptions used by the Pico SDK.
 */
#ifndef _BOARDS_RASTER_CUSTOM_H
#define _BOARDS_RASTER_CUSTOM_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

#define RASPBERRYPI_RASTER_CUSTOM

/*
 * This custom board is intended to map to the RP2350B/RP2354B package family.
 * The SDK comments explicitly describe PICO_RP2350A as the selector for the 30 GPIO
 * A package and 0 for the 48 GPIO B package. Use 0 to expose the wider package.
 */
#define PICO_RP2350A 0

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif

#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif

#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

#endif /* _BOARDS_RASTER_CUSTOM_H */
