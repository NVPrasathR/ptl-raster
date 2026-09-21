# Firmware architecture

Raster Pick to Light is a static-allocation C11 Pico SDK application with a
cooperative 10 ms service loop.

## Data flow

1. Bounded HTTP parsing routes REST requests to the pharmacy protocol.
2. Pharmacy JSON is completely validated into temporary assignments before state
   mutation. `led_list` is authoritative; conflicting legacy `leds` data is rejected.
3. Ten channel framebuffers retain per-LED RGB, team, and on/off state.
4. A dirty-channel scheduler serializes channels through one PIO state machine and
   DMA channel, preserving scarce RP235x PIO resources.
5. Configuration uses version, CRC-32, generation counters, and two storage slots.
6. OTA accepts a bounded stream only through authentication, staging, target,
   integrity, authenticity, pending-boot, and health-confirm backend contracts.

## Modules

- `application/`: colors, shelf mapping, channel state, pharmacy protocol.
- `drivers/`: PIO/DMA WS2812, SSD1306-gated OLED, button, buzzer, status LED.
- `network/`: bounded HTTP routing and nonblocking W5500 network state.
- `configuration/`: validated persistent model and transactional storage abstraction.
- `ota/`: SHA-256 validation and fail-closed update state machine.
- `web/`: deterministic embedded offline dashboard asset.

The integrated target uses about 141 KiB BSS
including HTTP request/response buffers. Maximum channel length is 192 LEDs;
default is 138, grouped in shelves of
six. No unbounded heap allocation is used in the application modules.

## Hardware boundary

The RP235x SDK target, PIO program, W5500 SPI/TCP server, and application compile
for the assumed B package. W5500 DHCP lease exchange, nonvolatile flash placement, production
signature verification, bootloader slot switching, SSD1306 identity/address, and
electrical characteristics require the custom board and cannot be accepted by
software compilation alone.
