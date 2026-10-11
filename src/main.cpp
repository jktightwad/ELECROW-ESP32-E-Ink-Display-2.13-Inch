#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "board_pins.h"
#include "crowpanel_epd.h"
#include "line_store.h"
#include "net.h"
#include "text_layout.h"

// Display controller: this unit matches Elecrow's "arduino-v1.2" example code
// (BUSY low while busy, black/white mode with MCU-loaded waveforms). GxEPD2's
// SSD1680 and JD79661 drivers both failed on it, so a small driver based on
// Elecrow's v1.2 sequence is used instead (src/crowpanel_epd.cpp).

CrowPanelEPD epd;
GFXcanvas1 canvas(CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT);  // 1 = black
LineStore lines;

const uint8_t TRIGGER_BUTTON = KEY_OK;   // press the dial
const int16_t MARGIN = 6;

// Requests from MQTT are queued here and handled in loop(), so the slow
// display refresh never runs inside the MQTT callback.
bool pending_random = false;
bool pending_text = false;
String pending_text_value;

// Draws `text` centred on the visible landscape area and refreshes the panel.
void showText(const char* text) {
  canvas.setRotation(1);                 // landscape: 250 x 128, visible y = 0..121
  canvas.fillScreen(0);
  drawWrappedText(canvas, text, MARGIN, MARGIN,
                  CrowPanelEPD::HEIGHT - 2 * MARGIN,
                  CrowPanelEPD::VISIBLE_WIDTH - 2 * MARGIN);
  canvas.setRotation(0);

  epd.begin();                           // wakes the panel from deep sleep
  uint32_t t = millis();
  bool ok = epd.display(canvas.getBuffer());
  epd.sleep();
  Serial.printf("Refresh %s after %lu ms\n", ok ? "finished" : "TIMED OUT",
                (unsigned long)(millis() - t));
}

// Picks a random line, never the same one twice in a row.
void showRandomLine() {
  static int last = -1;
  if (lines.size() == 0) return;
  int i = random(lines.size());
  if (lines.size() > 1) {
    while (i == last) i = random(lines.size());
  }
  last = i;
  Serial.printf("Showing line %d: %s\n", i, lines.get(i).c_str());
  showText(lines.get(i).c_str());
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  pinMode(TRIGGER_BUTTON, INPUT_PULLUP);   // reads LOW when pressed (per Elecrow's example)
  lines.begin();

  NetHandlers h;
  h.onLines = [](const String& text) {
    if (lines.replaceAll(text)) Serial.printf("List replaced: %u lines\n", (unsigned)lines.size());
  };
  h.onAdd = [](const String& line) {
    if (lines.add(line)) {
      Serial.printf("Line added (%u total)\n", (unsigned)lines.size());
      netPublishLines(lines.toText());   // keep Adafruit IO's copy of the list in step
    }
  };
  h.onShowRandom = []() { pending_random = true; };
  h.onShowText = [](const String& text) { pending_text_value = text; pending_text = true; };
  netBegin(h);

  showRandomLine();
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
  }
  delay(10);
}
