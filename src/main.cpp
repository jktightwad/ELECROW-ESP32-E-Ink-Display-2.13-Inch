#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "board_pins.h"
#include "crowpanel_epd.h"
#include "line_store.h"
#include "net.h"
#include "screen.h"

// Display controller: this unit matches Elecrow's "arduino-v1.2" example code
// (BUSY low while busy, black/white mode with MCU-loaded waveforms). GxEPD2's
// SSD1680 and JD79661 drivers both failed on it, so a small driver based on
// Elecrow's v1.2 sequence is used instead (src/crowpanel_epd.cpp).

CrowPanelEPD epd;
GFXcanvas1 canvas(CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT);  // 1 = black
LineStore lines;

const uint8_t TRIGGER_BUTTON = KEY_OK;            // press the dial
const uint32_t MESSAGE_SHOW_MS = 10000;           // how long a message stays before the dashboard returns
const uint32_t DASHBOARD_MIN_REDRAW_MS = 30000;   // limit status-change redraws (each one flashes the panel)

// What the panel is showing.
enum class Screen { Dashboard, Message };
Screen screen = Screen::Dashboard;
uint32_t message_until = 0;         // when to return to the dashboard
uint32_t dashboard_drawn_at = 0;
String dashboard_status;            // status text the dashboard was last drawn with

// Requests from MQTT are queued here and handled in loop(), so the slow
// display refresh never runs inside the MQTT callback.
bool pending_random = false;
bool pending_text = false;
String pending_text_value;

// Refreshes the panel with what is on the canvas.
void refresh() {
  epd.begin();                           // wakes the panel from deep sleep
  uint32_t t = millis();
  bool ok = epd.display(canvas.getBuffer());
  epd.sleep();
  Serial.printf("Refresh %s after %lu ms\n", ok ? "finished" : "TIMED OUT",
                (unsigned long)(millis() - t));
}

const char* stateText(NetState s) {
  switch (s) {
    case NetState::Connected:  return "connected";
    case NetState::Connecting: return "connecting...";
    default:                   return "off";
  }
}

// Status summary; the dashboard is redrawn when this changes.
String currentStatus() {
  return String(stateText(netWifiState())) + "|" + stateText(netMqttState()) + "|" +
         netIp() + "|" + String((unsigned long)lines.size());
}

void showDashboard() {
  String ip = netIp();
  DashboardInfo info = {stateText(netWifiState()), stateText(netMqttState()), ip.c_str(), lines.size()};
  canvas.setRotation(1);                 // landscape: 250 x 128, visible y = 0..121
  drawDashboardScreen(canvas, info);
  canvas.setRotation(0);
  Serial.println("Showing dashboard");
  refresh();
  screen = Screen::Dashboard;
  dashboard_status = currentStatus();
  dashboard_drawn_at = millis();
}

// Shown messages stay for MESSAGE_SHOW_MS (counted from the end of the refresh).
void startMessageTimer() {
  screen = Screen::Message;
  message_until = millis() + MESSAGE_SHOW_MS;
}

// Shows `text` on its own (MQTT "show" command).
void showText(const char* text) {
  canvas.setRotation(1);
  if (!drawTextScreen(canvas, text)) Serial.println("Text too long - shortened");
  canvas.setRotation(0);
  refresh();
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
  refresh();
  startMessageTimer();
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  pinMode(TRIGGER_BUTTON, INPUT_PULLUP);   // reads LOW when pressed (per Elecrow's example)
  lines.begin();

  NetHandlers h;
  h.onAdd = [](const String& line) {
    if (lines.add(line)) Serial.printf("Line added (%u total)\n", (unsigned)lines.size());
  };
  h.onShowRandom = []() { pending_random = true; };
  h.onShowText = [](const String& text) { pending_text_value = text; pending_text = true; };
  netBegin(h);

  showDashboard();
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

  if (pending_text) {
    pending_text = false;
    showText(pending_text_value.c_str());
  } else if (pending_random) {
    pending_random = false;
    showRandomLine();
  } else if (screen == Screen::Message && (int32_t)(millis() - message_until) >= 0) {
    showDashboard();
  } else if (screen == Screen::Dashboard && currentStatus() != dashboard_status &&
             millis() - dashboard_drawn_at >= DASHBOARD_MIN_REDRAW_MS) {
    showDashboard();                     // e.g. Wi-Fi/MQTT connected after boot
  }
  delay(10);
}
