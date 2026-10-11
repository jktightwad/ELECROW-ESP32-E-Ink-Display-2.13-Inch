#include <Arduino.h>
#include "board_pins.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Elecrow's init code targets the SSD1680 (122x250). GxEPD2 has several SSD1680
// 122x250 classes; if the image is garbled or faint, try GxEPD2_213_GDEY0213B74
// or GxEPD2_213_BN. Elecrow also lists a JD79661 variant, which these classes
// would not drive.
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

  // Diagnostic: SSD1680 BUSY is HIGH while busy, LOW when idle.
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
