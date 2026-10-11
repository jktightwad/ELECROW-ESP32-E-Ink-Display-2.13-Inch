#include <Arduino.h>
#include "board_pins.h"

#include <SPI.h>
#include <GxEPD2_4C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Controller: JD79661. Elecrow's README lists two possible controllers (SSD1680Z,
// JD79661). Hardware tests on this unit showed BUSY is LOW while busy and HIGH when
// idle, which matches GxEPD2's JD79661 driver and not its SSD1680 drivers (those
// timed out on every refresh). GxEPD2 supports the JD79661 as the GDEY0213F51 panel,
// a 4-colour 122x250 display; this unit is sold as black/white, so only
// black and white are drawn.
// If a future unit has the SSD1680 instead, use:
//   #include <GxEPD2_BW.h>
//   GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(...));
GxEPD2_4C<GxEPD2_213c_GDEY0213F51, GxEPD2_213c_GDEY0213F51::HEIGHT> display(
    GxEPD2_213c_GDEY0213F51(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

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

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("CrowPanel 2.13 desk display - boot");

  // Configure the control pins before GxEPD2 touches them. GxEPD2 writes to
  // them before calling pinMode, which this ESP32 core rejects with an error.
  pinMode(EPD_CS, OUTPUT);   digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_DC, OUTPUT);   digitalWrite(EPD_DC, HIGH);
  pinMode(EPD_RST, OUTPUT);  digitalWrite(EPD_RST, HIGH);
  pinMode(EPD_BUSY, INPUT);

  pinMode(EPD_POWER, OUTPUT);
  digitalWrite(EPD_POWER, HIGH);   // e-paper power rail on (see board_pins.h)
  delay(100);                      // let the panel power up; Elecrow waits 100ms before reset

  // Diagnostic: on the JD79661, BUSY is LOW while busy and HIGH when idle.
  Serial.printf("BUSY after power-on: %d\n", digitalRead(EPD_BUSY));

  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200);   // also enables GxEPD2 diagnostic output on Serial
  Serial.printf("BUSY after init: %d\n", digitalRead(EPD_BUSY));
  drawHello();
  Serial.println("Display drawn");
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("alive %lu\n", (unsigned long)n++);
  delay(5000);
}
