#pragma once
#include <Arduino.h>

// Wi-Fi + MQTT. Topics (prefix set in secrets.h, default "desk"):
//   <prefix>/lines          retained; whole list, one entry per line -> replaces the list
//   <prefix>/lines/add      one line -> appended to the list
//   <prefix>/show/random    any payload -> show a random line
//   <prefix>/show/text      text -> show that text once
//   <prefix>/status         published by the display: "online" / "offline" (retained)
struct NetHandlers {
  void (*onLines)(const String& text);
  void (*onAdd)(const String& line);
  void (*onShowRandom)();
  void (*onShowText)(const String& text);
};

// Returns false if there is no secrets.h (network features off).
bool netBegin(const NetHandlers& handlers);
void netLoop();

// Publishes the list to <prefix>/lines as a retained message, so the broker's
// copy matches the device after a change made with /lines/add.
void netPublishLines(const String& text);
