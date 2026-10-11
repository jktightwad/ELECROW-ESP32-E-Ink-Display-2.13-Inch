#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <time.h>
#include "board_pins.h"
#include "crowpanel_epd.h"
#include "line_store.h"
#include "location_store.h"
#include "net.h"
#include "screen.h"
#include "weather.h"

// Display controller: this unit matches Elecrow's "arduino-v1.2" example code
// (BUSY low while busy, black/white mode with MCU-loaded waveforms). GxEPD2's
// SSD1680 and JD79661 drivers both failed on it, so a small driver based on
// Elecrow's v1.2 sequence is used instead (src/crowpanel_epd.cpp).

CrowPanelEPD epd;
GFXcanvas1 canvas(CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT);  // 1 = black
LineStore lines;

const char* const DEFAULT_LOCATION = "Albany, NY";   // until changed with the "location" command
const uint8_t TRIGGER_BUTTON = KEY_OK;               // press the dial
const uint8_t SETUP_BUTTON = KEY_EXIT;               // hold while starting to open Wi-Fi setup
const uint32_t MESSAGE_SHOW_MS = 10000;              // how long a message stays up
const uint32_t WEATHER_EVERY_MS = 10 * 60000UL;      // weather refresh (with a full screen refresh)
const uint32_t WEATHER_RETRY_MS = 2 * 60000UL;       // retry after a failed fetch
const uint32_t FULL_REFRESH_EVERY_MS = 10 * 60000UL; // clears ghosting left by fast refreshes
const uint32_t STALE_AFTER_MS = 30 * 60000UL;        // show "offline" when weather is this old

enum class Screen { Dashboard, Message };
Screen screen = Screen::Dashboard;
uint32_t message_until = 0;

Location location;
WeatherData weather;
uint32_t weather_ok_at = 0;          // millis() of the last successful fetch
uint32_t weather_tried_at = 0;
bool weather_tried = false;

// Last frame sent to the panel, needed for fast refreshes.
uint8_t prev_frame[CrowPanelEPD::BUFFER_BYTES];
bool prev_valid = false;
uint32_t last_full_at = 0;
long drawn_minute = -1;              // local minute shown on the dashboard

// Requests from MQTT are queued here and handled in loop(), so the slow
// display refresh never runs inside the MQTT callback.
bool pending_random = false;
bool pending_text = false;
String pending_text_value;
String pending_location;

// ---------------------------------------------------------------- display

// Sends the canvas to the panel: full refresh, or fast refresh of changed pixels.
void refresh(bool full) {
  epd.begin();                           // wakes the panel from deep sleep
  uint32_t t = millis();
  bool fast = !full && prev_valid;
  bool ok = fast ? epd.displayFast(canvas.getBuffer(), prev_frame) : epd.display(canvas.getBuffer());
  epd.sleep();
  memcpy(prev_frame, canvas.getBuffer(), sizeof prev_frame);
  prev_valid = true;
  if (!fast) last_full_at = millis();
  Serial.printf("%s refresh %s after %lu ms\n", fast ? "Fast" : "Full", ok ? "finished" : "TIMED OUT",
                (unsigned long)(millis() - t));
}

// ---------------------------------------------------------------- time

// Local time needs both NTP (UTC) and the location's UTC offset from Open-Meteo.
bool haveTime() { return time(nullptr) > 1700000000 && weather.valid; }

long currentMinute() { return haveTime() ? (long)((time(nullptr) + weather.utc_offset) / 60) : -1; }

// ---------------------------------------------------------------- screens

void showDashboard(bool full) {
  DashboardInfo info = {};
  info.location = location.label.c_str();
  info.weather = &weather;
  info.stale = weather.valid && millis() - weather_ok_at > STALE_AFTER_MS;
  info.have_time = haveTime();
  if (info.have_time) {
    time_t t = time(nullptr) + weather.utc_offset;
    struct tm tm;
    gmtime_r(&t, &tm);
    info.year = tm.tm_year + 1900;
    info.month = tm.tm_mon + 1;
    info.day = tm.tm_mday;
    info.weekday = tm.tm_wday;
    info.hour = tm.tm_hour;
    info.minute = tm.tm_min;
  }
  canvas.setRotation(1);                 // landscape: 250 x 128, visible y = 0..121
  drawDashboardScreen(canvas, info);
  canvas.setRotation(0);
  refresh(full);
  screen = Screen::Dashboard;
  drawn_minute = currentMinute();
}

void startMessageTimer() {
  screen = Screen::Message;
  message_until = millis() + MESSAGE_SHOW_MS;
}

// Shows `text` on its own (MQTT "show" command).
void showText(const char* text) {
  canvas.setRotation(1);
  if (!drawTextScreen(canvas, text)) Serial.println("Text too long - shortened");
  canvas.setRotation(0);
  refresh(true);
  startMessageTimer();
}

// Picks a random line, never the same one twice in a row, and shows it under
// the "Tell David:" heading.
void showRandomLine() {
  static int last = -1;
  if (lines.size() == 0) return;
  int i = random(lines.size());
  if (lines.size() > 1) {
    while (i == last) i = random(lines.size());
  }
  last = i;
  Serial.printf("Showing line %d: %s\n", i, lines.get(i).c_str());
  canvas.setRotation(1);
  if (!drawDavidScreen(canvas, lines.get(i).c_str())) Serial.println("Line too long - shortened");
  canvas.setRotation(0);
  refresh(true);
  startMessageTimer();
}

void showSetup(const char* ap_name) {
  canvas.setRotation(1);
  drawSetupScreen(canvas, ap_name);
  canvas.setRotation(0);
  refresh(true);
}

// ---------------------------------------------------------------- weather

// Looks up the location if needed, then fetches the weather. Returns true on success.
bool updateWeather() {
  weather_tried = true;
  weather_tried_at = millis();
  String error;
  if (!location.valid) {
    Location loc;
    if (!geocode(location.label.length() ? location.label : String(DEFAULT_LOCATION), loc, error)) {
      Serial.printf("Location lookup failed: %s\n", error.c_str());
      return false;
    }
    location = loc;
    saveLocation(location);
  }
  WeatherData w;
  if (!fetchWeather(location, w, error)) {
    Serial.printf("Weather fetch failed: %s\n", error.c_str());
    return false;
  }
  weather = w;
  weather_ok_at = millis();
  Serial.printf("Weather: %.1fF code %d, UTC offset %lds\n", weather.temp, weather.code, weather.utc_offset);
  return true;
}

bool weatherDue() {
  if (!netWifiConnected()) return false;
  if (!weather_tried) return true;
  uint32_t interval = weather.valid ? WEATHER_EVERY_MS : WEATHER_RETRY_MS;
  return millis() - weather_tried_at >= interval;
}

// Handles a "location" command: look the place up, save it and refresh.
void changeLocation(const String& place) {
  Location loc;
  String error;
  if (!geocode(place, loc, error)) {
    Serial.printf("Location \"%s\" not changed: %s\n", place.c_str(), error.c_str());
    return;
  }
  location = loc;
  saveLocation(location);
  weather.valid = false;               // old location's data
  updateWeather();
  if (screen == Screen::Dashboard) showDashboard(true);
}

// ---------------------------------------------------------------- setup / loop

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  pinMode(TRIGGER_BUTTON, INPUT_PULLUP);   // buttons read LOW when pressed (per Elecrow's example)
  pinMode(SETUP_BUTTON, INPUT_PULLUP);
  bool force_portal = digitalRead(SETUP_BUTTON) == LOW;
  if (force_portal) Serial.println("Back button held - opening Wi-Fi setup");

  lines.begin();                           // also mounts the file system
  if (!loadLocation(location)) {
    location.label = DEFAULT_LOCATION;
    location.label.toUpperCase();
    location.valid = false;                // looked up once Wi-Fi is up
  }
  showDashboard(true);                     // placeholders until data arrives

  NetHandlers h;
  h.onAdd = [](const String& line) {
    if (lines.add(line)) Serial.printf("Line added (%u total)\n", (unsigned)lines.size());
  };
  h.onShowRandom = []() { pending_random = true; };
  h.onShowText = [](const String& text) { pending_text_value = text; pending_text = true; };
  h.onLocation = [](const String& place) { pending_location = place; };
  h.onPortal = [](const char* ap_name) { showSetup(ap_name); };
  netBegin(h, force_portal);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");   // UTC; the local offset comes from Open-Meteo
}

void loop() {
  netLoop();

  // Dial press (HIGH -> LOW), with a simple debounce.
  static int prev = HIGH;
  int now = digitalRead(TRIGGER_BUTTON);
  if (prev == HIGH && now == LOW) {
    delay(30);
    if (digitalRead(TRIGGER_BUTTON) == LOW) pending_random = true;
  }
  prev = now;

  if (pending_location.length()) {
    String place = pending_location;
    pending_location = "";
    changeLocation(place);
  } else if (pending_text) {
    pending_text = false;
    showText(pending_text_value.c_str());
  } else if (pending_random) {
    pending_random = false;
    showRandomLine();
  } else if (screen == Screen::Message) {
    if ((int32_t)(millis() - message_until) >= 0) {
      if (weatherDue()) updateWeather();
      showDashboard(true);
    }
  } else if (!weather.valid && weatherDue()) {
    // First data after boot (or after a failure): don't wait for a minute boundary.
    if (updateWeather()) showDashboard(true);
  } else if (currentMinute() != drawn_minute) {
    // New minute: update the clock. Every 10 minutes also refresh the weather
    // and do a full refresh to clear ghosting; otherwise a fast refresh.
    bool full = millis() - last_full_at >= FULL_REFRESH_EVERY_MS;
    if (weatherDue()) {
      updateWeather();
      full = true;
    }
    showDashboard(full);
  }
  delay(10);
}
