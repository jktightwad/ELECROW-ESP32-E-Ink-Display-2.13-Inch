#pragma once
#include <Arduino.h>

// Wi-Fi + MQTT.
//
// Wi-Fi (same approach as the On Air Light): on boot, try the credentials saved
// on the board, then WIFI_SSID/WIFI_PASSWORD from secrets.h if present, then
// open a setup hotspot (WiFiManager portal, open network "CrowPanel-Setup",
// page at 192.168.4.1) for up to 5 minutes. Holding the Back button while the
// board starts forces the setup hotspot.
//
// MQTT (Adafruit IO, needs secrets.h): subscribes to one shared feed
// (<user>/feeds/<MQTT_FEED>) and acts only on JSON commands addressed to
// DEVICE_ID; see command.h for the format.
struct NetHandlers {
  void (*onAdd)(const String& line);
  void (*onShowRandom)();
  void (*onShowText)(const String& text);
  void (*onLocation)(const String& place);
  void (*onPortal)(const char* ap_name);   // setup hotspot opened: show instructions
};

// Connects Wi-Fi (may block up to ~6 minutes if the setup portal opens), then
// starts MQTT if configured. `force_portal` opens the setup hotspot straight away.
void netBegin(const NetHandlers& handlers, bool force_portal);
void netLoop();

bool netWifiConnected();
