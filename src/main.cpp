#include <Arduino.h>
#include "board_pins.h"

// Stage 1: confirm toolchain, upload and serial monitor work.
// The display code is enabled once board_pins.h is verified (PINS_VERIFIED = 1),
// so an unverified pin map can't drive the wrong GPIOs.

#if PINS_VERIFIED
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Driver class depends on the panel's controller IC (SSD1680 vs SSD1683 etc.),
// which is not yet confirmed. GxEPD2_213_B74 is GxEPD2's SSD1680 122x250 class.
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
    GxEPD2_213_B74(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void drawHello() {
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("Desk display");
    display.setCursor(10, 60);
    display.print("Hello!");
  } while (display.nextPage());
  display.hibernate();
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  pinMode(EPD_POWER, OUTPUT);
  digitalWrite(EPD_POWER, HIGH);   // e-paper power rail on (see board_pins.h)

#if PINS_VERIFIED
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200);
  drawHello();
  Serial.println("Display drawn");
#else
  Serial.println("Pins not verified yet - display code disabled");
#endif
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("alive %lu\n", (unsigned long)n++);
  delay(5000);
}
