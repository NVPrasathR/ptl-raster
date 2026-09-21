# Build guide

## Prerequisites
- macOS toolchain with `cmake` and `arm-none-eabi-gcc`
- Local Pico SDK available at `/System/Volumes/Data/private/tmp/pico-sdk`
- Optional: `ninja` for faster builds (CMake will still configure with the default generator)

## Detecting the SDK
The project attempts to discover the SDK in this order:
1. `PICO_SDK_PATH` environment variable
2. `PICO_SDK_PATH` CMake cache value
3. `/System/Volumes/Data/private/tmp/pico-sdk`

If the SDK is in another location, set:

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

## Configure and build
```bash
cmake -S . -B build -G Ninja \
  -DPICO_SDK_PATH=/System/Volumes/Data/private/tmp/pico-sdk \
  -DPICO_BOARD=raster_custom
cmake --build build
```

## Build script
The repository includes `tools/build.sh`, which performs the same check while defaulting to the local SDK path.

```bash
./tools/build.sh
```

## Flashing
The project also includes `tools/flash.sh`, which loads the generated UF2 onto a USB-connected RP235x board.

```bash
./tools/flash.sh
```

## Notes
- The default target uses the custom board header `cmake/raster_custom.h`.
- This board header selects the RP235x B package; custom PCB electrical and flash
  details remain subject to schematic validation.
