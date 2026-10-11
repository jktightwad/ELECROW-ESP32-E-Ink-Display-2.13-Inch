#pragma once
#include <Arduino.h>

// Wi-Fi + MQTT, set up for Adafruit IO. Feeds (keys set in secrets.h):
//   FEED_LINES   whole list, one entry per line -> replaces the list
//   FEED_ADD     one line -> appended to the list
//   FEED_SHOW    "random" / "1" / empty -> random line; other text -> shown once
//   FEED_STATUS  optional; the display publishes "online" when it connects
// Topics are <user>/feeds/<key>. On connect the display requests the latest
// FEED_LINES value with Adafruit IO's <topic>/get.
struct NetHandlers {
  void (*onLines)(const String& text);
  void (*onAdd)(const String& line);
  void (*onShowRandom)();
  void (*onShowText)(const String& text);
};

// Returns false if there is no secrets.h (network features off).
bool netBegin(const NetHandlers& handlers);
void netLoop();

// Publishes the list to FEED_LINES, so Adafruit IO's copy matches the device
// after a change made with FEED_ADD.
void netPublishLines(const String& text);
