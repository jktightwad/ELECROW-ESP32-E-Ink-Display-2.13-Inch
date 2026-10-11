#pragma once
#include <Arduino.h>

// Wi-Fi + MQTT.
//
// Wi-Fi: on boot, scan and join the strongest known network: those saved on the
// board (/wifi.txt, see wifi_store.h) plus WIFI_NETWORKS / WIFI_SSID from
// secrets.h. If none is in range, open a setup hotspot (WiFiManager portal,
// open network "CrowPanel-Setup", page at 192.168.4.1) for up to 5 minutes;
// a network set up there is saved. Holding the Back button while the board
// starts forces the setup hotspot. If Wi-Fi drops for over a minute, known
// networks are rescanned every minute.
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
bool netMqttEnabled();      // Adafruit IO details present in secrets.h
bool netMqttConnected();
