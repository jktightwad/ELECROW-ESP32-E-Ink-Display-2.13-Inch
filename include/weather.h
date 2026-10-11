#pragma once
#include <Arduino.h>

// Current conditions and today's summary, as returned by Open-Meteo
// (temperatures in °F, wind in mph; see weather.cpp).
struct WeatherData {
  bool valid = false;
  float temp = 0, feels = 0, high = 0, low = 0;
  int humidity = 0;           // %
  float wind = 0;             // mph
  int wind_dir = 0;           // degrees the wind comes from
  int precip_prob = 0;        // today's maximum chance of precipitation, %
  int code = -1;              // WMO weather code
  bool is_day = true;
  char sunrise[6] = "";       // local "HH:MM"
  char sunset[6] = "";
  long utc_offset = 0;        // seconds, for the location's local time
};

struct Location {
  String label;               // shown on screen, e.g. "ALBANY, NY"
  float lat = 0, lon = 0;
  bool valid = false;
  bool automatic = true;      // follow the internet connection's location (IP lookup)
};

// Looks up a place name ("Albany, NY") with Open-Meteo's geocoding API.
// Returns false and sets `error` on failure.
bool geocode(const String& name, Location& out, String& error);

// Approximate location of the internet connection (ipinfo.io, no account).
// Usually city-level; can point to the internet provider's hub town.
bool locateByIp(Location& out, String& error);

// Fetches current weather for `loc`. Returns false and sets `error` on failure.
bool fetchWeather(const Location& loc, WeatherData& out, String& error);

// WMO code -> short label that fits the screen, and icon character (WI:: fonts).
const char* weatherLabel(int code);
char weatherIcon(int code, bool is_day);

// 16-point compass direction for a wind bearing.
const char* compass(int degrees);
