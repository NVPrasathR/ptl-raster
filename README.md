# Raster Pick to Light

Embedded C11 firmware for a 10-channel pharmacy shelf-location controller built
around the Raspberry Pi RP235x family, W5500 Ethernet, and WS2812-compatible LEDs.

## Implemented software

- Ten independent static LED framebuffers, 138 LEDs/channel by default and 192 max.
- Six-LED shelf grouping with correct final partial-shelf handling.
- Per-LED color, brightness, team ID, and on/off state.
- Single-state-machine PIO plus DMA WS2812 channel scheduler.
- Strict, atomic pharmacy JSON validation with current and legacy formats.
- Bounded HTTP parser/router and documented REST API.
- DHCP/static network state and nonblocking link-recovery model.
- Versioned, CRC-protected, two-slot persistent configuration abstraction.
- SSD1306-gated OLED, nonblocking buzzer, debounced button, and status LED drivers.
- Offline embedded dashboard for control, configuration, diagnostics, and OTA upload.
- SHA-256-validated, authenticated, staged, fail-closed OTA state machine.
- Host test suite, sanitizer support, CI, and RP235x CMake build.

## Build

Requirements: Pico SDK 2.3.0 or compatible, ARM GNU toolchain, CMake, Ninja, and
Python 3.

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
./tools/build.sh
```

Artifacts are written under `build/firmware/`:

- `raster_pick_to_light.elf`
- `raster_pick_to_light.bin`
- `raster_pick_to_light.uf2`

Run portable tests with:

```bash
./tools/test.sh
```

See [BUILD](docs/BUILD.md), [API](docs/API.md), [architecture](docs/ARCHITECTURE.md),
[hardware](docs/HARDWARE.md), [GPIO map](docs/GPIO_MAP.md), [OTA](docs/OTA.md),
[security](docs/SECURITY.md), and [testing](docs/TESTING.md).

## Hardware verification status

The software compiles for the Pico SDK `rp2350-arm-s` platform using the 48-GPIO
B-package assumption. No physical custom PCB was available. GPIO multiplexing and
electrical design, W5500 SPI/socket operation, OLED controller/address, buzzer
type, LED color order/timing and power distribution, persistent flash regions,
secure boot, OTA rollback, and power-failure recovery remain mandatory physical
bring-up tests. The OTA path fails closed without board-provided staging,
signature, and boot-control backends.

The target includes a direct W5500 SPI/TCP HTTP server and uses the configured
fallback address (`192.168.1.250`) while DHCP is selected. A wire-level DHCP lease
client is not included in this hardware-unverified revision; the state remains
`dhcp_wait` rather than falsely reporting a lease. Persistent configuration writes
also fail closed until the custom-board flash regions are confirmed and a storage
backend is installed.

At the theoretical WS2812 worst case of 60 mA per pixel, 1,380 LEDs could require
about 82.8 A at 5 V (414 W), excluding conversion and wiring losses. The default
brightness is limited to 32/255, but the installation still requires engineered
power injection, fusing, conductor sizing, level shifting, grounding, and measured
current limits.

## Security

The embedded API is HTTP, not HTTPS. Production deployment requires a segmented
trusted network, provisioned administrative credentials, authorization on
configuration and OTA operations, and a production signature-verification root.
Never commit signing private keys or production credentials.

## License

No repository license has been selected. All rights are reserved. Pico SDK and
toolchain dependencies retain their respective upstream licenses.
