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
- [x] Display driver (`src/crowpanel_epd.cpp`), based on Elecrow's v1.2 example; orientation and visible area confirmed on hardware
- [x] Random "Tell David" line on dial press, list stored on the board (LittleFS)
- [x] Weather dashboard, clock, fast refresh, startup checklist, status page
- [x] Over-the-air updates: v1.0.0 installed over Wi-Fi from a signed GitHub release (dev -> 1.0.0)

## Buttons
| Button | Action |
|---|---|
| Dial press | random "Tell David" line (10 s) |
| Home (Elecrow: Menu, GPIO2): the **top** button on the side | status page (10 s): Wi-Fi network and signal, IP, MQTT, location, weather age, uptime, MAC |
| Back (Exit, GPIO1): with the USB-C port at the bottom, the **bottom** button on the side | back to the dashboard now; **hold 5 s**: restart into the Wi-Fi setup hotspot |
| Dial up/down | not used yet |

## Firmware updates (OTA)
Displays update themselves from this repository's GitHub Releases:
- They check the latest release at start-up, every 6 hours, and on
  `{"deviceId":"nodavid","cmd":"update"}`. A newer version is downloaded into the spare firmware
  slot, its **signature** is checked, and the display restarts into it ("Downloading..." on screen).
  Saved data (lines, Wi-Fi networks, location, settings) is kept.
- If the new firmware can't hold a Wi-Fi connection within 5 minutes, or keeps restarting, the display
  switches back to the previous firmware and skips that version until an `update` command.
  (A firmware that crashes before it starts running can't recover this way; that needs USB.)
- Status reports go to the `debug-messages` feed as JSON, e.g.
  `{"type":"nodavid-status","fw":"1.3.0","event":"ota-updated","detail":"from 1.2.0"}`.
  Events: `boot`, `ota-updated`, `ota-failed`, `ota-rolled-back`, `ota-check` (reply to the command).
- Builds from your PC report version `dev` and only update on the `update` command.

**One-time setup**
1. In the PlatformIO terminal: `python tools/make_ota_key.py`. It writes `include/ota_public_key.h`
   (commit this) and prints a **private key**: add it as a GitHub repository secret named
   `OTA_SIGNING_KEY` (Settings → Secrets and variables → Actions), then clear it from your screen.
   Never commit or share it: anyone with it can sign firmware the displays will accept.
2. Flash each display once over USB with firmware that includes that public key.

**Releasing**: commit, then `git tag v1.3.0` and `git push origin v1.3.0` (or create the tag in
GitHub Desktop / a GitHub release). The *Release firmware* workflow builds, signs and publishes it.

## Startup
On power-up the display shows a **startup checklist** under a "POWERING ON" header: Wi-Fi, Clock,
Location, Weather, Messages (MQTT). Each step is ticked (or crossed, with a short reason) as it completes, using fast
refreshes, then the dashboard appears. Failed steps are retried in the background.

## Wi-Fi setup
At start the display scans and joins the **strongest known network in range**. Known networks are:
- networks saved on the board (`/wifi.txt`, up to 10, most recently used first). Every network it
  connects to is saved, including ones set up through the hotspot, so they survive reboots and
  firmware uploads (but not *Upload Filesystem Image*, which replaces the file system);
- networks listed in `WIFI_NETWORKS` in `secrets.h` (see `secrets.example.h`). A build from your PC
  also saves these on the board, so release builds (which have no `secrets.h`) know them.

If none is in range, it opens an open hotspot **CrowPanel-Setup** for 5 minutes and shows the steps on
screen: join it from a phone, pick your network on the page that opens (or browse to 192.168.4.1).
The same page has fields for the **Adafruit IO username and key**; they're saved on the board
(`/config.txt`). A build from your PC that has them in `secrets.h` copies them there automatically. Leave the key field empty to keep the saved key. The hotspot is open (no password),
so enter the key somewhere private.
To add a network, or change the Adafruit IO details, **hold the Back button for 5 seconds**: the
display restarts into the setup hotspot.
If Wi-Fi drops for more than a minute, it rescans for any known network every minute.

Passwords are stored unencrypted in the board's flash (as the ESP32's own Wi-Fi storage does), so
anyone with the board and a USB cable could read them.

## Weather dashboard
- Data: [Open-Meteo](https://open-meteo.com) (free for non-commercial use, no API key): current
  temperature, feels-like, today's high/low, humidity, wind, chance of precipitation, sunrise/sunset.
  Units: °F and mph.
- Clock updates every minute with a **fast refresh** (only changed pixels). Every 10 minutes the
  weather is fetched and the screen gets a **full refresh**, which also clears fast-refresh ghosting.
- Shows "offline" if the weather is more than 30 minutes old.
- Location: **automatic by default**: looked up from the internet connection's IP address
  ([ipinfo.io](https://ipinfo.io), no account) at every start and every 6 hours, so a moved board
  follows. IP location is approximate (usually the right city, sometimes the internet provider's
  hub town). If the lookup fails, Albany, NY is used.
  To fix a location instead: `{"deviceId":"nodavid","cmd":"location","text":"Saratoga Springs, NY"}`
  (looked up with Open-Meteo's geocoding; "Town, ST" works). To go back to automatic:
  `{"deviceId":"nodavid","cmd":"location","text":"auto"}`. The choice is saved on the board.
- Fonts and icons are generated from TrueType files by `tools/make_fonts.py` (needs Python + Pillow);
  the generated headers in `include/fonts/` are committed, so you only re-run it to change sizes or icons.

## Random lines
The list lives in `data/lines.txt`: plain text, one entry per line. Curly quotes, "…" and
dashes are converted for the display font; other non-ASCII characters (accents, emoji) are dropped.

- **Load the list onto the board:** PlatformIO sidebar → *Project Tasks* → *crowpanel_213* →
  *Platform* → **Upload Filesystem Image**. Until then the built-in copy in `include/lines.h` is used.
- **Show a line:** press the dial, or send a `random` command over MQTT (below). It appears under a
  "Tell David:" heading for 10 seconds (`MESSAGE_SHOW_MS` in `src/main.cpp`), then the dashboard returns.
  MQTT `show` text gets the same 10 seconds.
- **Dashboard:** the weather dashboard (below).

### MQTT (Adafruit IO, shared feed)
The display listens on an existing feed (`MQTT_FEED` in `secrets.h`, default `onair-dot-control`)
and acts only on JSON commands addressed to its `DEVICE_ID` (default `nodavid`), in the same
style as the On Air Light firmware. Everything else on the feed is ignored silently.

| Value sent to the feed | Effect |
|---|---|
| `{"deviceId":"nodavid","cmd":"random"}` | shows a random line |
| `{"deviceId":"nodavid","cmd":"show","text":"Back in 5"}` | shows that text once |
| `{"deviceId":"nodavid","cmd":"add","text":"A new line"}` | adds the line to the list stored on the board |
| `{"deviceId":"nodavid","cmd":"location","text":"Albany, NY"}` | fixes the weather location |
| `{"deviceId":"nodavid","cmd":"location","text":"auto"}` | location from the IP address (default) |

`"deviceIds":["nodavid", ...]` works too. The On Air Light firmware ignores these, because
its device ID doesn't match.
- Adafruit IO error and throttle messages (`<user>/errors`, `<user>/throttle`) are printed to the serial monitor.
- Lines added over MQTT exist only on the board. Running *Upload Filesystem Image* again replaces them with `data/lines.txt`, so copy any keepers into that file.
- Port 1883 is unencrypted, so the AIO key travels in plain text on your network.

## Notes
- `include/secrets.h` (optional, git-ignored): extra Wi-Fi networks and local-build fallbacks; see `include/secrets.example.h`. Release builds don't use it.

## License
GPL-3.0. See [LICENSE](LICENSE).

`src/crowpanel_epd.cpp` contains display register values and waveform tables taken from
Elecrow's example code for this board, which is published without a license.

Fonts: Barlow (SIL OFL 1.1, `tools/fonts/OFL-Barlow.txt`) and Weather Icons by Erik Flowers
(SIL OFL 1.1, `tools/fonts/OFL-WeatherIcons.txt`); `include/fonts/` holds bitmaps generated from them.
