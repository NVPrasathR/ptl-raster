# Hardware assumptions and verification notes

## RP2350/RP2354 family facts
The local Pico SDK headers and board definitions indicate the RP235x family is treated as `rp2350` in the build system, with package variants expressed by the `PICO_RP2350A` macro.

Evidence in the local SDK:
- `$PICO_SDK_PATH/src/rp2350/pico_platform/include/pico/platform.h`
- `$PICO_SDK_PATH/src/rp2350/hardware_regs/include/hardware/platform_defs.h`
- `$PICO_SDK_PATH/src/boards/include/boards/pico2.h`

The SDK comments describe:
- `PICO_RP2350A == 1`: 30 GPIO A package
- `PICO_RP2350A == 0`: 48 GPIO B package

The B package is therefore the only safe assumption for GPIO 30, 31, and 32. Those GPIO numbers are not available on the 30 GPIO A package.

## RP2354B and custom-board distinction
The product brief references `RP2354B`, but the Pico SDK naming is still `rp2350` and `PICO_RP2350A` package flags. This means the silicon family is validated through the SDK, while the exact board-level part number and flash layout remain custom-board assumptions until the schematic or vendor data confirms them.

## What is verified here
- RP235x family supported by the Pico SDK
- 30 GPIO vs 48 GPIO package distinction
- GPIO 30/31/32 valid on the 48 GPIO B package
- B-package assumption used in `cmake/raster_custom.h`
- The firmware now includes an SH1106G-compatible 128x64 I2C OLED manager on
  GPIO20 (SDA), GPIO21 (SCL), address `0x3c`, and 400 kHz I2C. The controller
  address, display variant, pull-ups, and electrical levels still require
  confirmation on the assembled board.

## What is intentionally not asserted here
- Board-specific flash type or density
- Electrical and schematic correctness of the product-specified map in `firmware/board_pins.h`
- Final Ethernet, OLED, buzzer, and LED-chain wiring assignments
- Measured DHCP lease and renewal behavior on the final switch/router topology

## Power and signal integrity

The default 1,380-pixel installation has a theoretical 82.8 A, 5 V worst case
at 60 mA per RGB pixel. Brightness limiting reduces commanded duty but is not a
substitute for measured load analysis. Use distributed, fused power injection,
adequate conductor sizing, common grounding, bulk/local decoupling, and verified
3.3 V-to-5 V data-level compatibility. Confirm WS2812D-F8 timing and wire color
order from the purchased component datasheet.

## Authoritative references
- Raspberry Pi Pico SDK local headers (above)
- https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
- https://datasheets.raspberrypi.com/rp2354/rp2354b-datasheet.pdf

## W5500 DHCP software evidence
The firmware now vendors the minimal WIZnet ioLibrary W5500/DHCP sources under
`third_party/wiznet/` and uses them on-device for hardware DHCP negotiation. This
proves build-time integration of a real DHCP client, but not successful leasing on the
final custom hardware without physical network testing.
