# ESP32 E-Ink Desk Display

Firmware for the **Elecrow CrowPanel ESP32 E-Paper HMI 2.13"** (122x250, black/white, ESP32-S3), built with PlatformIO in VS Code.

## Setup
1. Install VS Code and the **PlatformIO IDE** extension.
2. Clone this repo, then use *File > Open Folder* to open it. PlatformIO detects `platformio.ini`.
3. Connect the board over USB, then click **Upload** (→) and **Serial Monitor** (plug icon) in the status bar.

## Display notes
- Draw on a `GFXcanvas1(128, 250)` with colour 1 = black, then pass its buffer to `CrowPanelEPD::display()`.
- In landscape (`setRotation(1)`, 250 x 128) only y = 0..121 is visible.

## Status
- [x] Upload and serial output working
- [x] Pin map in `include/board_pins.h` taken from Elecrow's official example code
- [x] Display driver (`src/crowpanel_epd.cpp`), based on Elecrow's v1.2 example; orientation and visible area confirmed on hardware
- [x] Random line on dial press, list stored on the board (LittleFS)
- [ ] MQTT: confirm on hardware with your broker
- [ ] Wi-Fi + content (clock/weather/etc.)

## Random lines
The list lives in `data/lines.txt`: plain text, one entry per line. Curly quotes, "…" and
dashes are converted for the display font; other non-ASCII characters (accents, emoji) are dropped.

- **Load the list onto the board:** PlatformIO sidebar → *Project Tasks* → *crowpanel_213* →
  *Platform* → **Upload Filesystem Image**. Until then the built-in copy in `include/lines.h` is used.
- **Show a line:** press the dial, or send `nodavid:random` over MQTT (below).

### MQTT (Adafruit IO, shared feed)
The display listens on an existing feed (`MQTT_FEED` in `secrets.h`, default `onair-dot-control`)
and only acts on values that start with `nodavid:`; anything else on the feed is ignored.

| Value sent to the feed | Effect |
|---|---|
| `nodavid:random` | shows a random line |
| `nodavid:show <text>` | shows that text once |
| `nodavid:add <line>` | adds the line to the list stored on the board |

- Any other device subscribed to the same feed also receives these values, so it must ignore ones it doesn't recognise.
- Lines added over MQTT exist only on the board. Running *Upload Filesystem Image* again replaces them with `data/lines.txt`, so copy any keepers into that file.
- Port 1883 is unencrypted, so the AIO key travels in plain text on your network.

## Notes
- Wi-Fi and MQTT settings: copy `include/secrets.example.h` to `include/secrets.h` and fill it in. `secrets.h` is git-ignored, so it is never pushed. Without it, the display runs with the dial button only.

## License
GPL-3.0. See [LICENSE](LICENSE).

`src/crowpanel_epd.cpp` contains display register values and waveform tables taken from
Elecrow's example code for this board, which is published without a license.
