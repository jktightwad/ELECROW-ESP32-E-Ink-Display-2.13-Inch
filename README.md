# ESP32 E-Ink Desk Display

Firmware for the **Elecrow CrowPanel ESP32 E-Paper HMI 2.13"** (122x250, black/white, ESP32-S3), built with PlatformIO in VS Code.

## Setup
1. Install VS Code and the **PlatformIO IDE** extension.
2. Clone this repo, then use *File > Open Folder* to open it. PlatformIO detects `platformio.ini`.
3. Connect the board over USB, then click **Upload** (→) and **Serial Monitor** (plug icon) in the status bar.

## Status
- [ ] Stage 1: upload works and serial prints `alive N`
- [x] Pin map in `include/board_pins.h` taken from Elecrow's official example code
- [ ] Draw "Hello" on the panel (GxEPD2 SSD1680 driver; confirm on hardware)
- [ ] Wi-Fi + content (clock/weather/etc.)

## Notes
- Wi-Fi credentials go in `include/secrets.h`, which is git-ignored.

## License
GPL-3.0. See [LICENSE](LICENSE). This matches the license of GxEPD2, the display library this project uses.
