#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "board_pins.h"
#include "crowpanel_epd.h"

// Display controller: this unit matches Elecrow's "arduino-v1.2" example code
// (BUSY low while busy, black/white mode with MCU-loaded waveforms). GxEPD2's
// SSD1680 and JD79661 drivers both failed on it, so a small driver based on
// Elecrow's v1.2 sequence is used instead (src/crowpanel_epd.cpp).

CrowPanelEPD epd;
GFXcanvas1 canvas(CrowPanelEPD::WIDTH, CrowPanelEPD::HEIGHT);  // 1 = black

void drawHello() {
  canvas.fillScreen(0);
  canvas.drawRect(0, 0, 122, 250, 1);     // outline of the visible area (122 px wide)
  canvas.setRotation(1);                  // landscape: 250 x 128
  canvas.setFont(&FreeMonoBold9pt7b);
  canvas.setTextColor(1);
  canvas.setCursor(10, 30);
  canvas.print("Desk display");
  canvas.setCursor(10, 60);
  canvas.print("Hello!");
  canvas.setRotation(0);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  epd.begin();
  drawHello();
  uint32_t t = millis();
  bool ok = epd.display(canvas.getBuffer());
  Serial.printf("Refresh %s after %lu ms\n", ok ? "finished" : "TIMED OUT",
                (unsigned long)(millis() - t));
  epd.sleep();
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("alive %lu\n", (unsigned long)n++);
  delay(5000);
}
