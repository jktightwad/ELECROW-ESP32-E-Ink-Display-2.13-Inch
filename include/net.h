#pragma once
#include <Arduino.h>

// Wi-Fi + MQTT, set up for Adafruit IO. The display subscribes to one shared
// feed (<user>/feeds/<MQTT_FEED>, set in secrets.h) and acts only on JSON
// commands addressed to DEVICE_ID; see command.h for the format.
struct NetHandlers {
  void (*onAdd)(const String& line);
  void (*onShowRandom)();
  void (*onShowText)(const String& text);
};

// Returns false if there is no secrets.h (network features off).
bool netBegin(const NetHandlers& handlers);
void netLoop();
