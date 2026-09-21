# GPIO map

The map below is the product-specified custom PCB assignment centralized in
`firmware/board_pins.h`. It compiles for the 48-GPIO B package but remains subject
to schematic and electrical review.

| Function | GPIO |
|---|---:|
| WS2812 channels C01-C10 | 0-9 |
| W5500 MISO / CS / SCLK / MOSI | 16 / 17 / 18 / 19 |
| OLED SDA / SCL | 20 / 21 |
| Push button | 26 |
| Status LED | 28 |
| W5500 reset / interrupt | 30 / 31 |
| Buzzer | 32 |

The local Pico SDK 2.3.0 defines 30 GPIO for `PICO_RP2350A == 1` and 48
otherwise. GPIO 30-32 therefore require an RP2350B/RP2354B package. Pin
multiplexing, pull resistors, LED polarity, buzzer drive circuitry, W5500 level
compatibility, and I2C pull-ups must be checked against the final schematic.
