# HTTP and pharmacy API

Maximum request body size is 4096 bytes. LED numbers are one-based on the wire and
validated against each channel's configured length. A failed command does not
partially change channel state. Omitted LEDs retain their prior state.

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/v1/device/info` | Identity, firmware, network and OTA state |
| GET | `/api/v1/channels` | Summaries for all channels |
| GET | `/api/v1/channels/C01` | One channel |
| POST | `/api/v1/led/control` | Apply validated pharmacy command |
| POST | `/api/v1/channels/C01/off` | Disable one channel |
| POST | `/api/v1/channels/all/off` | Disable all channels |
| GET/PUT | `/api/v1/config` | Read/update nonsecret settings |
| GET | `/api/v1/health` | Device health |
| GET | `/api/v1/ota/status` | Update state |
| POST | `/api/v1/ota/upload` | Board-backend-gated update stream |

Preferred command:

```json
{
  "channel": "C01",
  "status": "on",
  "led_list": [
    {"led_no": 1, "ledcolor": "RED", "team_id": "1"},
    {"led_no": 2, "ledcolor": "GREEN", "team_id": "2"}
  ]
}
```

Named colors are `WHITE`, `VIOLET`, `RED`, `GREEN`, `BLUE`, `YELLOW`, and
`OFF`, case-insensitively. `#RRGGBB` is supported. The legacy
`"leds":"1:RED|2:GREEN|"` representation remains accepted. When both forms are
present they must agree; they are never silently merged.

Administrative configuration and OTA routes require an authentication layer in
the final W5500 transport integration. HTTP provides no confidentiality; deploy
only on a trusted, segmented network unless a tested TLS gateway is used.
