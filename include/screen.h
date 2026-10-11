#pragma once
#include <Adafruit_GFX.h>

// Screen layouts, drawn on a landscape canvas (GFXcanvas1 in rotation 1:
// 250 x 128, of which y = 0..121 is visible). Colour 1 = black.

// "Tell David:" heading, then `line` word-wrapped and fitted below it.
// Returns false if the line had to be clipped.
bool drawDavidScreen(Adafruit_GFX& gfx, const char* line);

struct WeatherData;

// Weather dashboard.
struct DashboardInfo {
  const char* location;       // e.g. "ALBANY, NY"
  const WeatherData* weather; // nullptr or !valid -> placeholders
  bool have_time;             // false until NTP and the UTC offset are known
  int year, month, day, weekday, hour, minute;  // local time; weekday 0 = Sunday
  bool stale;                 // weather older than expected (shown as "offline")
};
void drawDashboardScreen(Adafruit_GFX& gfx, const DashboardInfo& info);

// "7:05 AM" from 24-hour hour/minute.
String formatTime12(int hour, int minute);

// Wi-Fi setup instructions while the setup hotspot is open.
void drawSetupScreen(Adafruit_GFX& gfx, const char* ap_name);

// `text` word-wrapped and fitted to the whole screen.
bool drawTextScreen(Adafruit_GFX& gfx, const char* text);
