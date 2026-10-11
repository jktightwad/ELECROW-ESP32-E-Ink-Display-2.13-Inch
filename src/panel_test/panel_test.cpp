// Standalone panel test for the CrowPanel 2.13" e-paper. Does not use GxEPD2.
//
// Drives the SSD1680 with bit-banged SPI (as Elecrow's examples do) and logs
// how long the BUSY line stays high after each step. Draws horizontal black and
// white bands if the panel completes a refresh.
//
// Built only by the "panel_test" PlatformIO environment (see platformio.ini).

#include <Arduino.h>
#include "board_pins.h"

static const uint16_t ROWS = 250;          // gate lines
static const uint16_t ROW_BYTES = 16;      // 122 px -> 16 bytes (128 bits)

static void writeByte(uint8_t b) {
  digitalWrite(EPD_CS, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(EPD_SCK, LOW);
    digitalWrite(EPD_MOSI, (b & 0x80) ? HIGH : LOW);
    digitalWrite(EPD_SCK, HIGH);
    b <<= 1;
  }
  digitalWrite(EPD_CS, HIGH);
}

static void command(uint8_t c) {
  digitalWrite(EPD_DC, LOW);
  writeByte(c);
  digitalWrite(EPD_DC, HIGH);
}

static void data(uint8_t d) {
  digitalWrite(EPD_DC, HIGH);
  writeByte(d);
}

// Waits while BUSY is HIGH. Returns true if it went LOW within timeout_ms.
static bool waitBusy(const char* step, uint32_t timeout_ms) {
  uint32_t start = millis();
  while (digitalRead(EPD_BUSY) == HIGH) {
    if (millis() - start > timeout_ms) {
      Serial.printf("  %-12s BUSY still HIGH after %lu ms (TIMEOUT)\n", step, (unsigned long)timeout_ms);
      return false;
    }
    delay(1);
  }
  Serial.printf("  %-12s BUSY low after %lu ms\n", step, (unsigned long)(millis() - start));
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== CrowPanel 2.13 panel test (no GxEPD2) ===");

  pinMode(EPD_SCK, OUTPUT);
  pinMode(EPD_MOSI, OUTPUT);
  pinMode(EPD_CS, OUTPUT);   digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_DC, OUTPUT);   digitalWrite(EPD_DC, HIGH);
  pinMode(EPD_RST, OUTPUT);  digitalWrite(EPD_RST, HIGH);
  pinMode(EPD_BUSY, INPUT);

  Serial.printf("BUSY before power-on: %d\n", digitalRead(EPD_BUSY));
  pinMode(EPD_POWER, OUTPUT);
  digitalWrite(EPD_POWER, HIGH);
  delay(100);
  Serial.printf("BUSY 100ms after power-on: %d\n", digitalRead(EPD_BUSY));

  // Hardware reset
  digitalWrite(EPD_RST, LOW);  delay(10);
  digitalWrite(EPD_RST, HIGH); delay(10);
  waitBusy("hw reset", 3000);

  command(0x12);                       // software reset
  waitBusy("sw reset", 3000);

  command(0x01);                       // driver output control: 250 gate lines
  data(0xF9); data(0x00); data(0x00);
  command(0x11); data(0x03);           // data entry: X+, Y+
  command(0x44); data(0x00); data(0x0F);                          // RAM X 0..15
  command(0x45); data(0x00); data(0x00); data(0xF9); data(0x00);  // RAM Y 0..249
  command(0x3C); data(0x05);           // border waveform
  command(0x18); data(0x80);           // internal temperature sensor
  command(0x4E); data(0x00);           // RAM X counter
  command(0x4F); data(0x00); data(0x00);  // RAM Y counter
  waitBusy("init", 3000);

  // Image: bands 25 rows tall, alternating black (0x00) and white (0xFF)
  command(0x24);
  for (uint16_t y = 0; y < ROWS; y++) {
    uint8_t v = ((y / 25) % 2) ? 0xFF : 0x00;
    for (uint16_t x = 0; x < ROW_BYTES; x++) data(v);
  }
  Serial.println("  image data sent");

  command(0x22); data(0xF7);           // full update sequence
  command(0x20);                       // activate
  bool ok = waitBusy("refresh", 20000);

  command(0x10); data(0x01);           // deep sleep
  Serial.println(ok ? "RESULT: refresh completed - check the screen for stripes"
                    : "RESULT: refresh did not complete");
}

void loop() {
  delay(1000);
}
