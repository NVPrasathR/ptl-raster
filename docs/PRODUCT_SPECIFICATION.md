
# RASTER PICK TO LIGHT
# COMPLETE EMBEDDED C DEVELOPMENT PROJECT
# GITHUB COPILOT DESKTOP APPLICATION
# SENIOR EMBEDDED FIRMWARE DEVELOPER MASTER PROMPT

============================================================
1. YOUR ROLE AND RESPONSIBILITIES
============================================================

You are my Lead Senior Embedded C Firmware Developer,
Embedded Systems Architect, Hardware Integration Engineer,
Network Engineer, Web UI Developer, OTA Firmware Engineer,
and Quality Assurance Engineer.

I am using the GitHub Copilot desktop application on macOS.

Your task is to create a complete, production-oriented
embedded firmware project named:

RASTER PICK TO LIGHT

Company Name: Raster
Product Name: Pick to Light

The application will be used in a pharmacy to identify
the physical shelf location of medication using
addressable RGB LEDs.

You are responsible for:

1. Creating the complete project.
2. Designing the embedded firmware architecture.
3. Writing actual Embedded C source code.
4. Implementing all hardware drivers.
5. Implementing W5500 Ethernet communication.
6. Implementing pharmacy JSON integration.
7. Implementing all 10 WS2812 LED channels.
8. Creating a complete embedded web dashboard.
9. Implementing web-based OTA firmware updates.
10. Implementing persistent device configuration.
11. Creating automated tests.
12. Building and validating the firmware.
13. Preparing GitHub repository documentation.
14. Preparing firmware release artifacts.

Do not stop after generating an architecture document,
README, project skeleton, or example code.

Continue through actual implementation, integration,
compilation, testing, and documentation.

Do not claim that a feature is complete unless the
corresponding implementation exists and has been verified
to the extent possible in the available environment.

============================================================
2. GITHUB COPILOT AGENT AND COWORKER INSTRUCTIONS
============================================================

Use the available capabilities of the GitHub Copilot
desktop application.

If supported, use coding agents, subagents, task delegation,
parallel development, and coworker capabilities.

Act as the lead developer and coordinate the following
engineering responsibilities.

AGENT 1: EMBEDDED SYSTEM ARCHITECT

Responsibilities:

- Verify the exact MCU and board configuration.
- Verify the GPIO mapping.
- Verify PIO and DMA resources.
- Design the firmware architecture.
- Define the memory and flash layout.
- Design the main application scheduler.
- Review real-time and memory requirements.

AGENT 2: LED AND HARDWARE ENGINEER

Responsibilities:

- Implement all 10 WS2812 output channels.
- Implement individual LED control.
- Implement shelf grouping.
- Implement brightness and color control.
- Implement OLED display.
- Implement buzzer.
- Implement push button.
- Implement status LED.

AGENT 3: ETHERNET AND API ENGINEER

Responsibilities:

- Implement W5500 SPI communication.
- Implement DHCP and static IP.
- Implement HTTP server.
- Implement pharmacy JSON API.
- Implement network recovery.
- Implement API validation and error handling.

AGENT 4: WEB UI AND OTA ENGINEER

Responsibilities:

- Design the embedded web dashboard.
- Implement channel configuration pages.
- Implement LED control pages.
- Implement network configuration pages.
- Implement firmware upload interface.
- Implement secure OTA update architecture.

AGENT 5: TESTING AND INTEGRATION ENGINEER

Responsibilities:

- Implement unit tests.
- Implement integration tests.
- Verify firmware compilation.
- Review memory safety.
- Review network security.
- Review firmware update recovery.
- Prepare hardware testing procedures.

If separate agents are unavailable, perform these
responsibilities sequentially within the main agent.

Do not invent unavailable agent capabilities.

Maintain a development plan and task checklist.

Create or update the project files as development progresses.

============================================================
3. PROJECT CREATION AND GITHUB
============================================================

Create a new project named:

raster-pick-to-light

Create a local project workspace if the desktop application
has access to the local filesystem.

If the desktop application only supports remote development,
create the project in the available development environment.

Use Git for version control.

Prepare a public GitHub repository named:

raster-pick-to-light

Use my connected GitHub account if repository creation
is available and authorized.

Before publishing the repository:

- Obtain my approval.
- Check for passwords and API keys.
- Check for private firmware signing keys.
- Check for confidential pharmacy information.
- Check for internal network credentials.
- Check third-party library licenses.
- Do not publish proprietary source code without authorization.

Do not invent a software license.

If GitHub repository creation is unavailable, prepare
the complete project locally and provide instructions
for publishing it.

Do not claim that the public repository has been created
unless GitHub confirms successful creation.

============================================================
4. DEVELOPMENT ENVIRONMENT
============================================================

Development computer:

Apple Mac running macOS.

Development language:

Embedded C using the C11 standard.

Build system:

CMake.

Embedded SDK:

Raspberry Pi Pico SDK, using a verified version compatible
with the selected RP235x target.

Compiler:

ARM GNU Toolchain.

Use the existing installed Pico SDK and ARM compiler
if accessible.

Do not automatically replace a working toolchain
with an older version.

If the GitHub Copilot desktop environment cannot access
my local Mac toolchain, prepare the project for local
compilation using VS Code and Terminal.

Create all required build scripts and documentation.

Use actual hardware-compatible code rather than
Arduino sketches, MicroPython, or simulated firmware.

============================================================
5. HARDWARE CONFIGURATION
============================================================

The intended hardware consists of:

- RP2354B microcontroller
- W5500 Ethernet controller
- WS2812D-F8 addressable RGB LEDs
- I2C OLED display
- Buzzer
- Push button
- Status LED

IMPORTANT HARDWARE VERIFICATION:

The MCU was previously identified as RP2350B and
is now identified as RP2354B.

Verify the exact microcontroller, package, board
schematic, and flash configuration.

Check the official MCU datasheet and Pico SDK support.

Verify the availability of GPIO30, GPIO31, and GPIO32.

Verify the SPI and I2C pin-function assignments.

Verify the actual OLED controller and resolution.

Verify the exact WS2812D-F8 LED protocol, color order,
and electrical requirements.

Do not invent hardware specifications.

If any hardware detail cannot be verified, clearly
identify the missing information and isolate the
assumption in the board configuration layer.

Continue implementing hardware-independent modules
while awaiting hardware clarification.

============================================================
6. GPIO PIN MAPPING
============================================================

WS2812 LED CHANNELS:

GPIO0  = Channel 1
GPIO1  = Channel 2
GPIO2  = Channel 3
GPIO3  = Channel 4
GPIO4  = Channel 5
GPIO5  = Channel 6
GPIO6  = Channel 7
GPIO7  = Channel 8
GPIO8  = Channel 9
GPIO9  = Channel 10

W5500 ETHERNET:

GPIO16 = W5500 MISO
GPIO17 = W5500 SCS / CS
GPIO18 = W5500 SCLK
GPIO19 = W5500 MOSI

GPIO30 = W5500 RSTn
GPIO31 = W5500 INT

OLED DISPLAY:

GPIO20 = OLED SDA
GPIO21 = OLED SCL

PUSH BUTTON:

GPIO26 = Push Button

STATUS LED:

GPIO28 = Status LED

BUZZER:

GPIO32 = Buzzer

Create:

board_pins.h
board_config.h

All GPIO assignments must be centralized.

Do not scatter hardcoded GPIO numbers throughout
the firmware.

Validate the selected SPI peripheral and alternate
pin functions against the actual RP235x datasheet.

Do not assume that the custom PCB uses the same
peripheral pin assignments as a Raspberry Pi Pico board.

============================================================
7. APPLICATION PURPOSE
============================================================

This device will be installed in a pharmacy.

The pharmacy application sends JSON commands to the
Pick to Light controller over Ethernet.

The controller identifies the requested medication
location by illuminating LEDs installed on pharmacy
shelves.

Each physical shelf or LED strip contains six LEDs
by default.

The controller must support multiple teams and
different LED colors simultaneously.

For example:

Team 1 may use RED LEDs.

Team 2 may use GREEN LEDs.

Team 3 may use BLUE LEDs.

Multiple teams may be active within the same channel.

Each LED must be independently addressable.

The system must be capable of controlling all
10 channels without one channel interfering with
the others.

============================================================
8. LED HARDWARE ARCHITECTURE
============================================================

LED Type:

WS2812D-F8.

Number of channels:

10.

Default LEDs per channel:

138.

Default LEDs per physical strip:

6.

Default physical strips per channel:

23.

Total default LEDs:

1380.

Each channel is physically independent.

The default channel configuration is:

C01 = GPIO0  = 138 LEDs
C02 = GPIO1  = 138 LEDs
C03 = GPIO2  = 138 LEDs
C04 = GPIO3  = 138 LEDs
C05 = GPIO4  = 138 LEDs
C06 = GPIO5  = 138 LEDs
C07 = GPIO6  = 138 LEDs
C08 = GPIO7  = 138 LEDs
C09 = GPIO8  = 138 LEDs
C10 = GPIO9  = 138 LEDs

The number of LEDs must be configurable from the
web interface.

Each channel must support an independent LED count.

For example:

C01 = 138 LEDs
C02 = 120 LEDs
C03 = 96 LEDs
C04 = 138 LEDs

Support a configurable number of LEDs per physical strip.

Default:

6 LEDs per strip.

For channels whose LED count is not divisible by six,
handle the final partial strip correctly.

The maximum configurable LED count must be determined
from actual memory, driver, timing, and power constraints.

Do not permit configuration beyond the verified
hardware and firmware limits.

============================================================
9. LED COLOR CONFIGURATION
============================================================

Default color mapping for a six-LED physical strip:

LED 1:
WHITE
RGB(255,255,255)

LED 2:
VIOLET
RGB(255,0,255)

LED 3:
RED
RGB(255,0,0)

LED 4:
GREEN
RGB(0,255,0)

LED 5:
BLUE
RGB(0,0,255)

LED 6:
YELLOW
RGB(255,255,0)

Additional color:

OFF
RGB(0,0,0)

Implement a centralized color manager.

Support named colors:

WHITE
VIOLET
RED
GREEN
BLUE
YELLOW
OFF

Support case-insensitive color names.

Support custom RGB values if feasible.

Implement brightness control.

Brightness must be configurable from the web UI.

Implement a safe default brightness to reduce
power consumption and excessive LED current.

Do not assume that all 1380 LEDs can safely operate
at full brightness without verifying the power supply.

Document the expected maximum LED current and
power distribution requirements.

============================================================
10. LED DRIVER IMPLEMENTATION
============================================================

Implement an efficient WS2812 LED driver.

Use RP235x PIO and DMA where appropriate.

Investigate the available PIO state machines and
the feasibility of parallel output.

Do not assume that ten independent PIO state machines
are available.

Design a suitable 10-channel output architecture.

Requirements:

- Independent LED framebuffer for every channel.
- Independent channel ON/OFF control.
- Individual LED color control.
- Physical shelf grouping.
- Multiple team colors.
- Configurable brightness.
- Nonblocking LED updates.
- Efficient framebuffer refresh.
- Safe synchronization with network commands.

Avoid disabling interrupts for long periods.

Do not block the HTTP server while updating LEDs.

Verify the WS2812 timing requirements.

Verify the physical LED color order.

Implement a logical RGB interface and convert
to the actual wire color order in the driver.

============================================================
11. PHARMACY APPLICATION JSON INTEGRATION
============================================================

The pharmacy application will send JSON data.

Use HTTP REST communication over W5500 Ethernet.

The Pick to Light controller acts as the HTTP server.

Primary endpoint:

POST /api/v1/led/control

Content-Type: application/json

The pharmacy application will send data similar
to the following JSON.

{
  "teamcolor": "GREEN",
  "controlport": "172.17.1.251:3000",
  "channel": "C01",
  "status": "on",
  "leds": "1:RED|2:RED|3:RED|4:GREEN|5:GREEN|6:GREEN|7:RED|8:RED|9:RED|10:GREEN|11:GREEN|12:GREEN|",
  "led_list": [
    {
      "led_no": 1,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 2,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 3,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 4,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 5,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 6,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 7,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 8,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 9,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 10,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 11,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 12,
      "ledcolor": "GREEN",
      "team_id": "2"
    }
  ]
}

============================================================
12. JSON FIELD DEFINITIONS
============================================================

FIELD: channel

Identifies the physical LED channel.

Valid values:

C01 through C10.

Example:

"channel": "C01"

This identifies GPIO0 and the corresponding
physical LED channel.

FIELD: status

Controls whether the selected channel is ON or OFF.

Valid values:

"on"
"off"

When status is ON:

Apply the specified LED assignments.

When status is OFF:

Turn off the selected channel.

Do not turn off unrelated channels.

FIELD: teamcolor

Represents the default or team-level color.

Do not override explicitly assigned individual LED
colors unless the documented API rules require it.

FIELD: controlport

Example:

"controlport": "172.17.1.251:3000"

This is a pharmacy application endpoint reference.

Do not assume that it is the Pick to Light device IP.

Do not automatically connect to arbitrary addresses
supplied in this field.

Validate the field and document its intended usage.

FIELD: leds

Legacy compact LED assignment format.

Example:

"leds": "1:RED|2:RED|3:GREEN|"

FIELD: led_list

Recommended primary LED assignment format.

Each object contains:

led_no
ledcolor
team_id

Example:

{
  "led_no": 1,
  "ledcolor": "RED",
  "team_id": "1"
}

LED numbering starts at 1.

Internally, C arrays may use zero-based indexing.

Validate all LED numbers before accessing arrays.

============================================================
13. RECOMMENDED PHARMACY JSON FORMAT
============================================================

The preferred JSON format for new pharmacy
application development is:

{
  "channel": "C01",
  "status": "on",
  "led_list": [
    {
      "led_no": 1,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 2,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 3,
      "ledcolor": "RED",
      "team_id": "1"
    },
    {
      "led_no": 4,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 5,
      "ledcolor": "GREEN",
      "team_id": "2"
    },
    {
      "led_no": 6,
      "ledcolor": "GREEN",
      "team_id": "2"
    }
  ]
}

Use led_list as the authoritative representation.

Support the existing leds string for backward
compatibility.

If both led_list and leds are supplied:

- Prefer led_list.
- Do not silently merge conflicting assignments.
- Document the precedence rule.
- Report conflicting values where appropriate.

Implement strict JSON validation.

Reject invalid channels, invalid LED numbers,
invalid colors, malformed JSON, and oversized requests.

Parse and validate the complete command before
modifying the active LED state.

Define whether omitted LEDs retain their previous
state or are cleared.

Use a documented default behavior and keep it
consistent across the API and web interface.

============================================================
14. HTTP API ENDPOINTS
============================================================

Implement the following endpoints.

GET /api/v1/device/info

Return:

- Company name
- Product name
- Firmware version
- Hardware revision
- IP address
- MAC address
- Device uptime
- Network status
- OTA status

GET /api/v1/channels

Return the state of all 10 channels.

GET /api/v1/channels/C01

Return Channel 1 state.

POST /api/v1/led/control

Receive pharmacy JSON commands.

POST /api/v1/channels/C01/off

Turn off Channel 1.

POST /api/v1/channels/all/off

Turn off all channels.

GET /api/v1/config

Return nonsecret device configuration.

PUT /api/v1/config

Update validated device configuration.

GET /api/v1/health

Return device health information.

GET /api/v1/ota/status

Return firmware update status.

Implement consistent JSON responses.

Example successful response:

{
  "success": true,
  "channel": "C01",
  "status": "on",
  "updated_leds": 12,
  "message": "LED command applied"
}

Example error response:

{
  "success": false,
  "error": {
    "code": "INVALID_LED_NUMBER",
    "message": "LED number exceeds channel length"
  }
}

Use appropriate HTTP status codes.

Implement bounded request parsing.

Prevent buffer overflows and invalid memory access.

============================================================
15. MULTIPLE TEAM SUPPORT
============================================================

Support multiple pharmacy teams.

Example:

Team 1 = RED
Team 2 = GREEN
Team 3 = BLUE

Multiple teams can use the same physical channel.

Each LED must store:

- LED number
- RGB color
- Team identifier
- ON/OFF state

Do not force all LEDs in one channel to use
the same team color.

Support independent LED assignments.

The web dashboard must display the assigned
team and color for each LED.

============================================================
16. W5500 ETHERNET IMPLEMENTATION
============================================================

Implement the W5500 Ethernet controller.

Use a verified compatible WIZnet ioLibrary or
another suitable embedded networking implementation.

Implement:

- SPI initialization
- W5500 hardware reset
- W5500 initialization
- MAC address configuration
- DHCP
- Static IP
- Subnet mask
- Gateway
- DNS
- Ethernet link detection
- HTTP server
- Socket management
- Network timeout handling
- Network recovery

The device must support DHCP and static IP.

Default to DHCP unless an existing valid device
configuration specifies otherwise.

Allow the following network settings to be
configured through the web interface:

- DHCP enable/disable
- Static IP
- Subnet mask
- Gateway
- DNS server
- HTTP port
- Device name

Store network settings persistently.

Provide a recovery mechanism for incorrect
network configuration.

Do not block the entire application indefinitely
when Ethernet is disconnected.

The LED controller, OLED, button, and watchdog
must continue operating during network failure.

============================================================
17. OLED DISPLAY
============================================================

The OLED is connected through I2C.

GPIO20 = SDA
GPIO21 = SCL

Verify the actual OLED controller and resolution.

If the OLED uses SSD1306, implement a compatible
SSD1306 driver.

Display:

RASTER

PICK TO LIGHT

IP ADDRESS

FIRMWARE VERSION

Example:

+----------------------+
|        RASTER        |
|    PICK TO LIGHT     |
|                      |
| IP: 192.168.1.100    |
| FW: v1.0.0           |
+----------------------+

Adapt the display layout to the actual OLED
resolution.

During startup, show:

Raster
Pick to Light
Initializing...

After Ethernet initialization, show the assigned
IP address.

During network failure, show:

ETHERNET DISCONNECTED

During firmware update, show:

FIRMWARE UPDATE

During update failure, show an appropriate
error indication.

Use efficient display refreshes.

Do not block time-critical LED updates.

============================================================
18. BUZZER IMPLEMENTATION
============================================================

Buzzer GPIO:

GPIO32

When the device boots successfully, the buzzer
must generate a short confirmation signal.

Implement:

- Buzzer initialization
- Boot confirmation beep
- Nonblocking buzzer timing
- Configurable buzzer enable/disable
- Configurable boot beep duration

Use an appropriate driver for the actual buzzer type.

Do not assume an active buzzer and passive buzzer
have identical electrical requirements.

Avoid blocking delays.

============================================================
19. PUSH BUTTON IMPLEMENTATION
============================================================

Push Button GPIO:

GPIO26

Implement:

- GPIO initialization
- Software debouncing
- Short press detection
- Long press detection

Suggested behavior:

Short press:

Cycle through OLED information pages.

Long press:

Enter a documented maintenance mode.

Factory reset must require a deliberate action.

Do not allow an accidental short press to erase
network configuration or firmware.

Do not erase bootloader or firmware image data
during configuration reset.

============================================================
20. STATUS LED IMPLEMENTATION
============================================================

Status LED GPIO:

GPIO28

Implement the following indications.

BOOTING:

Slow blinking.

NETWORK CONNECTED:

Steady ON.

NETWORK DISCONNECTED:

Periodic blinking.

OTA IN PROGRESS:

Distinct update pattern.

CRITICAL ERROR:

Distinct error pattern.

Use nonblocking timing.

Make the LED polarity configurable according
to the actual board schematic.

============================================================
21. EMBEDDED WEB UI
============================================================

Create a complete responsive web interface.

The web UI must be served directly from the
embedded device over W5500 Ethernet.

Use:

HTML
CSS
Vanilla JavaScript

Avoid large JavaScript frameworks.

The interface must work without an internet
connection.

Optimize all assets for limited embedded flash
and network resources.

The web interface must include the following
pages or sections.

============================================================
22. WEB UI DASHBOARD
============================================================

Display:

Company: Raster

Product: Pick to Light

Device status

Firmware version

Hardware revision

IP address

MAC address

Ethernet connection status

Device uptime

Number of configured channels

Total configured LEDs

OTA update status

Provide clear navigation to all configuration
and diagnostic pages.

============================================================
23. WEB UI LED CONTROL
============================================================

Display all 10 channels.

Each channel must display:

- Channel number
- Configured LED count
- Number of physical strips
- Channel ON/OFF status
- Current brightness
- Individual LED colors
- Team assignments

Provide controls for:

- Channel ON
- Channel OFF
- All channels OFF
- Individual LED ON/OFF
- LED color selection
- Brightness adjustment
- LED test patterns

Display the physical shelf arrangement.

For a channel with 138 LEDs and six LEDs per strip,
display 23 physical strips.

Each physical strip must display six individual LEDs.

Allow the user to select a strip and inspect
its LED assignments.

Do not confuse the physical strip number with
the individual LED number.

============================================================
24. WEB UI CHANNEL CONFIGURATION
============================================================

Allow the user to configure:

- LED count per channel
- LEDs per physical strip
- Channel name
- Shelf labels
- Default LED colors
- Default brightness
- Channel enabled/disabled state

LED count must be independently configurable
for each channel.

Example:

C01 = 138 LEDs
C02 = 120 LEDs
C03 = 96 LEDs

Validate all values.

Prevent configuration beyond supported memory,
timing, and hardware limits.

Store validated configuration persistently.

============================================================
25. WEB UI NETWORK SETTINGS
============================================================

Provide configuration controls for:

DHCP

Static IP

Subnet mask

Gateway

DNS server

HTTP port

Device name

Display the current network configuration.

When network settings change, clearly inform
the user that the device may become accessible
at a different IP address.

Implement a documented recovery mechanism
for invalid settings.

============================================================
26. WEB UI PHARMACY JSON TESTING
============================================================

Create a JSON testing page.

Provide an editable JSON text area.

Allow the user to submit a JSON command directly
to the local device API.

Display:

- HTTP response
- Success or failure
- Validation errors
- Updated channel
- Updated LED count

Provide example JSON commands.

Include examples for:

- Turning on a channel.
- Turning off a channel.
- Assigning multiple LED colors.
- Assigning multiple teams.
- Turning off all channels.

The testing page must use the same API as the
pharmacy application.

Do not implement a separate mock-only LED
control mechanism.

============================================================
27. WEB UI OTA FIRMWARE UPDATE
============================================================

Create a firmware update page.

Display:

Current firmware version

Hardware revision

Firmware build information

Update status

Provide a firmware file selection control.

Provide an upload button.

Provide upload progress indication.

Provide image validation status.

Provide update progress.

Provide reboot status.

Provide meaningful error messages.

The firmware update must use the actual OTA
implementation rather than a simulated progress bar.

Do not claim update success until the device
has successfully booted and confirmed the new
firmware according to the implemented update protocol.

============================================================
28. OTA FIRMWARE ARCHITECTURE
============================================================

Implement Ethernet-based firmware updates.

The device must support updating firmware through
the web interface without requiring USB during
normal operation.

Before implementing OTA:

1. Verify the exact MCU.
2. Verify the flash configuration.
3. Verify flash capacity.
4. Verify the boot mechanism.
5. Verify firmware image size.
6. Determine the configuration storage region.
7. Determine safe firmware staging capacity.
8. Determine whether A/B firmware slots are feasible.
9. Determine the required bootloader architecture.

Do not assume that a UF2 file can automatically
be uploaded and executed as an Ethernet OTA image.

Design a suitable firmware image format.

The image must contain validated metadata including:

- Firmware version
- Target hardware identifier
- Image length
- Image integrity information
- Firmware authenticity information

Implement firmware authenticity verification
using a suitable cryptographic signature mechanism.

The signing private key must never be embedded
in the firmware or committed to GitHub.

OTA PROCESS:

1. Authenticate the update request.
2. Receive the firmware image.
3. Validate image size.
4. Validate target hardware compatibility.
5. Verify image integrity.
6. Verify firmware authenticity.
7. Stage the complete image safely.
8. Prepare the verified boot/update mechanism.
9. Reboot into the new firmware.
10. Perform application health verification.
11. Confirm successful firmware startup.
12. Roll back when supported and necessary.

Do not overwrite the active firmware before
establishing a safe recovery mechanism.

Do not implement OTA as an unsafe direct overwrite
of the running application.

Use bounded buffers and streaming upload.

Handle:

- Interrupted upload
- Invalid firmware
- Wrong hardware revision
- Flash programming failure
- Power interruption
- Failed firmware startup

If A/B OTA is not feasible on the verified hardware,
implement or propose a documented alternative
with a reliable recovery mechanism.

Do not claim rollback support unless the
implementation has been verified.

============================================================
29. PERSISTENT CONFIGURATION
============================================================

Implement persistent configuration storage.

Store:

- Network configuration
- LED count per channel
- Physical strip grouping
- Channel names
- Shelf labels
- Default LED colors
- Brightness settings
- Buzzer configuration
- Device settings

Use a versioned configuration structure.

Implement:

- Configuration validation
- Integrity checking
- Safe configuration writes
- Default configuration recovery
- Configuration migration where required

Avoid unnecessary flash writes.

Keep configuration storage separate from
firmware image storage.

Document the flash memory layout.

============================================================
30. SECURITY REQUIREMENTS
============================================================

The device will operate on a pharmacy network.

Implement appropriate security controls.

Requirements:

- Authenticated administrative web access
- Authorization for firmware updates
- Authorization for network configuration
- Authorization for factory reset
- Input validation
- Bounded JSON parsing
- Bounded HTTP request parsing
- Buffer overflow protection
- Safe network error handling
- Firmware authenticity verification

Do not hardcode production passwords.

Do not store private firmware signing keys
inside the device.

Do not commit secrets to GitHub.

Define a secure first-time administrative
credential provisioning process.

If HTTP is used without TLS, clearly document
that network traffic and credentials are not
protected against interception.

Do not claim HTTPS support unless TLS has
actually been implemented and tested.

Document the required trusted-network
deployment configuration.

============================================================
31. FIRMWARE PROJECT STRUCTURE
============================================================

Create a modular project structure similar to:

raster-pick-to-light/
|
|-- CMakeLists.txt
|-- README.md
|-- ARCHITECTURE.md
|-- HARDWARE.md
|-- GPIO_MAP.md
|-- API.md
|-- OTA.md
|-- BUILD.md
|-- TESTING.md
|-- SECURITY.md
|-- CHANGELOG.md
|-- .gitignore
|
|-- firmware/
|   |
|   |-- main.c
|   |-- board_pins.h
|   |-- board_config.h
|   |
|   |-- drivers/
|   |   |-- ws2812.c
|   |   |-- ws2812.h
|   |   |-- w5500_driver.c
|   |   |-- w5500_driver.h
|   |   |-- oled.c
|   |   |-- oled.h
|   |   |-- buzzer.c
|   |   |-- buzzer.h
|   |   |-- button.c
|   |   |-- button.h
|   |   |-- status_led.c
|   |   |-- status_led.h
|   |
|   |-- application/
|   |   |-- led_manager.c
|   |   |-- led_manager.h
|   |   |-- channel_manager.c
|   |   |-- channel_manager.h
|   |   |-- shelf_manager.c
|   |   |-- shelf_manager.h
|   |   |-- color_manager.c
|   |   |-- color_manager.h
|   |   |-- pharmacy_protocol.c
|   |   |-- pharmacy_protocol.h
|   |
|   |-- network/
|   |   |-- ethernet_manager.c
|   |   |-- ethernet_manager.h
|   |   |-- http_server.c
|   |   |-- http_server.h
|   |   |-- api_router.c
|   |   |-- api_router.h
|   |   |-- json_parser.c
|   |   |-- json_parser.h
|   |
|   |-- configuration/
|   |   |-- config_manager.c
|   |   |-- config_manager.h
|   |   |-- flash_storage.c
|   |   |-- flash_storage.h
|   |
|   |-- ota/
|   |   |-- ota_manager.c
|   |   |-- ota_manager.h
|   |   |-- firmware_validation.c
|   |   |-- firmware_validation.h
|   |   |-- boot_control.c
|   |   |-- boot_control.h
|   |
|   |-- system/
|       |-- system_manager.c
|       |-- system_manager.h
|       |-- diagnostics.c
|       |-- diagnostics.h
|       |-- watchdog.c
|       |-- watchdog.h
|
|-- web/
|   |-- index.html
|   |-- style.css
|   |-- app.js
|
|-- tests/
|   |-- test_led_manager.c
|   |-- test_json_parser.c
|   |-- test_channel_manager.c
|   |-- test_config_manager.c
|   |-- test_api.c
|
|-- tools/
|   |-- build.sh
|   |-- flash.sh
|   |-- package_ota.sh
|
|-- docs/
|
|-- build/

Adapt the structure where justified by the
actual implementation.

Do not create empty placeholder files merely
to match the proposed directory structure.

============================================================
32. MAIN FIRMWARE BOOT SEQUENCE
============================================================

Implement the following boot sequence:

1. Initialize system clocks.
2. Initialize required GPIOs.
3. Load persistent configuration.
4. Validate configuration.
5. Initialize LED output hardware.
6. Set LEDs to a safe startup state.
7. Initialize OLED.
8. Display Raster and Pick to Light.
9. Initialize buzzer.
10. Initialize status LED.
11. Initialize W5500.
12. Establish network configuration.
13. Obtain or apply the device IP address.
14. Display IP address and firmware version.
15. Start the HTTP server.
16. Start pharmacy JSON command processing.
17. Complete boot health checks.
18. Generate the successful boot buzzer signal.
19. Enter the main application loop.

Do not block indefinitely waiting for Ethernet
or DHCP.

The system must remain recoverable during
network initialization failure.

============================================================
33. MAIN APPLICATION LOOP
============================================================

The firmware must continuously process:

- Ethernet events
- HTTP requests
- Pharmacy JSON commands
- LED state updates
- LED output refresh
- OLED display updates
- Push-button events
- Buzzer timing
- Status LED indication
- Configuration changes
- OTA state
- Watchdog servicing

Avoid long blocking delays.

Use a cooperative scheduler, interrupt-driven
architecture, multicore design, or RTOS only
when justified by the actual requirements.

If multicore execution is used, define
synchronization and shared-data ownership.

Prevent concurrent modifications to active
LED framebuffers.

============================================================
34. MEMORY AND PERFORMANCE
============================================================

Use predictable memory allocation.

Prefer static allocation where practical.

Document:

- SRAM usage
- Flash usage
- LED framebuffer usage
- Network buffer usage
- Maximum JSON request size
- Maximum configurable LED count
- Expected LED refresh latency
- HTTP response latency under LED load

Avoid uncontrolled heap fragmentation.

Avoid unnecessary memory copies.

Implement bounded input buffers.

Do not claim real-time performance without
measurement or a documented timing analysis.

============================================================
35. BUILD SYSTEM AND FIRMWARE OUTPUT
============================================================

Create a complete CMake build system.

Use the verified Raspberry Pi Pico SDK
configuration for the target MCU.

Provide:

- Debug build
- Release build
- Toolchain configuration
- Board configuration
- Dependency management
- Firmware version information

Generate the appropriate artifacts for
the verified target.

These may include:

raster_pick_to_light.elf

raster_pick_to_light.bin

raster_pick_to_light.uf2

raster_pick_to_light_ota.bin

Only generate formats actually supported by
the selected target and boot architecture.

Do not assume that a normal UF2 file is
automatically compatible with the OTA system.

Create build instructions for macOS.

If the local compiler is inaccessible from
the Copilot desktop environment, document
the exact commands needed for local compilation.

============================================================
36. AUTOMATED TESTING
============================================================

Create host-side unit tests.

Test the following:

- Channel C01 through C10 validation
- Invalid channel identifiers
- LED number validation
- Configurable LED counts
- Physical strip mapping
- Color name parsing
- RGB value validation
- Team ID assignment
- Multiple teams in one channel
- JSON parsing
- Legacy leds string parsing
- led_list parsing
- Conflicting JSON assignments
- Status ON
- Status OFF
- Duplicate LED assignments
- Oversized JSON
- Empty LED list
- Configuration validation
- Configuration persistence
- API response generation
- OTA metadata validation

Create integration tests where possible.

Create documented physical hardware test procedures
for:

- All 10 LED channels
- 138 LEDs per channel
- OLED
- W5500 Ethernet
- DHCP
- Static IP
- Buzzer
- Push button
- Status LED
- Web UI
- OTA firmware update
- OTA recovery
- Power interruption
- Ethernet disconnection

Clearly distinguish:

COMPILED

HOST TESTED

HARDWARE TESTED

NOT YET VERIFIED

Do not claim that hardware testing has passed
unless testing was actually performed on the
physical board.

============================================================
37. DOCUMENTATION
============================================================

Create complete project documentation.

README.md:

Project overview, features, build instructions,
and quick start.

ARCHITECTURE.md:

Firmware architecture and module interactions.

HARDWARE.md:

Hardware requirements and electrical considerations.

GPIO_MAP.md:

Complete GPIO assignments.

API.md:

Pharmacy JSON protocol and HTTP API.

OTA.md:

Firmware update architecture, image format,
security, recovery, and update instructions.

BUILD.md:

macOS toolchain setup and build commands.

TESTING.md:

Automated and hardware testing procedures.

SECURITY.md:

Authentication, firmware signing, network security,
and credential provisioning.

CHANGELOG.md:

Firmware version history.

Include sample pharmacy JSON requests.

Document all configuration options.

============================================================
38. DEVELOPMENT PHASES
============================================================

PHASE 1:

Inspect the available project files and development
environment.

Verify the MCU, board, GPIO mapping, flash,
and SDK support.

PHASE 2:

Create the project architecture and CMake
build system.

PHASE 3:

Implement the WS2812 driver and all 10
LED channels.

PHASE 4:

Implement channel management, shelf mapping,
color management, and team assignments.

PHASE 5:

Implement W5500 Ethernet communication.

PHASE 6:

Implement the HTTP server and pharmacy JSON API.

PHASE 7:

Implement OLED, buzzer, button, and status LED.

PHASE 8:

Implement persistent configuration.

PHASE 9:

Implement the embedded web dashboard.

PHASE 10:

Implement the safe OTA firmware update system.

PHASE 11:

Integrate all firmware modules.

PHASE 12:

Compile the firmware and run automated tests.

PHASE 13:

Perform code review, security review, and
memory-safety review.

PHASE 14:

Prepare documentation and release artifacts.

PHASE 15:

Prepare the public GitHub repository and
publish it after receiving authorization.

At the end of each phase, report:

- Files created
- Files modified
- Features completed
- Build status
- Tests executed
- Remaining issues
- Next implementation phase

Continue working through the phases without
unnecessary interruptions.

Ask for clarification only when a missing hardware
detail, authorization, or architectural decision
prevents safe progress.

============================================================
39. ACCEPTANCE CRITERIA
============================================================

The project is considered complete only when
the following requirements have been implemented
and verified to the extent possible.

REQUIREMENT 1:

All 10 LED channels can be controlled independently.

REQUIREMENT 2:

Each channel supports 138 LEDs by default.

REQUIREMENT 3:

The LED count can be configured independently
for every channel through the web UI.

REQUIREMENT 4:

Physical shelf grouping supports six LEDs
per strip by default.

REQUIREMENT 5:

The pharmacy application can send JSON commands
over Ethernet.

REQUIREMENT 6:

The firmware correctly parses and validates
the pharmacy JSON format.

REQUIREMENT 7:

Multiple teams and multiple LED colors can
operate simultaneously.

REQUIREMENT 8:

The OLED displays Raster, Pick to Light,
the device IP address, and firmware version.

REQUIREMENT 9:

The buzzer generates a signal after successful boot.

REQUIREMENT 10:

The push button and status LED operate according
to the documented behavior.

REQUIREMENT 11:

The web UI supports LED control, channel
configuration, network settings, and API testing.

REQUIREMENT 12:

The web UI supports actual firmware upload
through the implemented OTA architecture.

REQUIREMENT 13:

Persistent configuration survives device reboot.

REQUIREMENT 14:

The firmware builds successfully for the
verified target MCU.

REQUIREMENT 15:

Automated tests pass.

REQUIREMENT 16:

Hardware-dependent features have documented
physical validation results or clearly identified
remaining hardware tests.

REQUIREMENT 17:

The project contains complete source code,
build instructions, and technical documentation.

REQUIREMENT 18:

The GitHub repository is prepared and published
when authorized and supported.

============================================================
40. START IMPLEMENTATION NOW
============================================================

You are the Lead Senior Embedded C Developer
responsible for the complete Raster Pick to Light
firmware project.

Start immediately.

FIRST:

Inspect the current workspace and available
development environment.

SECOND:

Verify the exact RP2354B hardware capabilities,
GPIO mapping, flash configuration, and SDK support.

THIRD:

Create the project directory and implementation plan.

FOURTH:

Create the actual Embedded C firmware project,
including the CMake build system.

FIFTH:

Begin implementing the hardware drivers and
application modules.

Continue through Ethernet, JSON API, OLED,
web UI, OTA, testing, and documentation.

Do not stop after generating a project proposal.

Do not produce only pseudocode.

Do not create empty placeholder functions
and describe them as completed features.

Write real, maintainable, compilable Embedded C.

If a hardware dependency prevents completion,
identify the exact missing information and
continue implementing independent modules.

Use the available GitHub Copilot desktop agent
capabilities to complete the project efficiently.

Maintain clear progress reports.

Do not claim production readiness without
appropriate compilation, testing, security
review, and physical hardware validation.

BEGIN THE RASTER PICK TO LIGHT PROJECT NOW.