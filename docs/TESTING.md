# Testing

## Host tests

Run the host-only firmware tests without the Pico SDK:

```sh
tools/test.sh
```

The script configures `tests/host` as an isolated C11 CMake project, compiles selected firmware modules with `-Wall -Wextra -Werror`, and runs CTest. On Linux, the host firmware library is also built with AddressSanitizer and UndefinedBehaviorSanitizer when the compiler accepts `-fsanitize=address,undefined`.

Covered host surfaces:

- Color parsing and custom RGB state.
- Channel `C01` through `C10`, invalid channel identifiers, LED bounds, and configurable LED counts.
- Shelf grouping, including a partial final shelf.
- Pharmacy JSON handling: strict `led_list`, legacy `leds`, multiple team assignments, precedence/conflict behavior, duplicate/oversize/malformed/empty bodies, `status=off`, and atomic failure preservation.
- HTTP request parsing, routing, status codes, and channel bounds.
- Configuration defaults/validation, two-slot persistence, CRC fallback, and failed-write recovery.
- W5500 DHCP/static/recovery state transitions.
- OTA digest, metadata, state transitions, interrupted uploads, wrong target/auth/backend/health failure paths.
- Dashboard asset presence.

The suite includes dashboard serving and legacy `leds` compatibility as normal
passing regression tests. There are no expected-failure tests.

## Optional Pico firmware build

The production firmware CMake project still requires the Raspberry Pi Pico SDK. To build it locally:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -DPICO_SDK_PATH="$PICO_SDK_PATH"
cmake --build build --parallel
```

The host tests above do not require `PICO_SDK_PATH` and are what CI runs.
