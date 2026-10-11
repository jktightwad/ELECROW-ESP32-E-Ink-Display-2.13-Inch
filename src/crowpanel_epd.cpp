#include "crowpanel_epd.h"
#include "board_pins.h"

// Register values and waveform tables are taken from Elecrow's example for this
// board: github.com/Elecrow-RD/CrowPanel-ESP32-2.13-E-paper-HMI-Display-with-122-250,
// example/arduino-v1.2/main/EPD_Init.cpp (EPD_Init, EPD_DisplayImage, EPD_Update,
// EPD_Sleep, lut_GC). The surrounding code is written for this project.

namespace {

// "GC" (full refresh) waveform. Each table is 56 bytes; only the first 8 are
// non-zero, so the rest are sent as zeros.
const uint8_t LUT_LEN = 56;
const uint8_t LUT_HEAD_LEN = 8;
const uint8_t LUT_VCOM[LUT_HEAD_LEN] = {0x01, 0x00, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};  // reg 0x20
const uint8_t LUT_WW[LUT_HEAD_LEN]   = {0x01, 0x60, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};  // reg 0x21
const uint8_t LUT_R22[LUT_HEAD_LEN]  = {0x01, 0x20, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
const uint8_t LUT_R23[LUT_HEAD_LEN]  = {0x01, 0x10, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
const uint8_t LUT_BB[LUT_HEAD_LEN]   = {0x01, 0x90, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};  // reg 0x24

}  // namespace

void CrowPanelEPD::writeByte(uint8_t b) {
  // Bit-banged SPI mode 0, as in Elecrow's example.
  digitalWrite(EPD_CS, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(EPD_SCK, LOW);
    digitalWrite(EPD_MOSI, (b & 0x80) ? HIGH : LOW);
    digitalWrite(EPD_SCK, HIGH);
    b <<= 1;
  }
  digitalWrite(EPD_CS, HIGH);
}

void CrowPanelEPD::command(uint8_t c) {
  digitalWrite(EPD_DC, LOW);
  writeByte(c);
  digitalWrite(EPD_DC, HIGH);
}

void CrowPanelEPD::data(uint8_t d) {
  digitalWrite(EPD_DC, HIGH);
  writeByte(d);
}

bool CrowPanelEPD::waitWhileBusy(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (digitalRead(EPD_BUSY) == LOW) {
    if (millis() - start > timeout_ms) return false;
    delay(1);
  }
  return true;
}

void CrowPanelEPD::begin() {
  pinMode(EPD_SCK, OUTPUT);
  pinMode(EPD_MOSI, OUTPUT);
  pinMode(EPD_CS, OUTPUT);   digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_DC, OUTPUT);   digitalWrite(EPD_DC, HIGH);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_BUSY, INPUT);

  pinMode(EPD_POWER, OUTPUT);
  digitalWrite(EPD_POWER, HIGH);

  // Hardware reset (timings from Elecrow's EPD_HW_SW_RESET)
  delay(100);
  digitalWrite(EPD_RST, HIGH); delay(10);
  digitalWrite(EPD_RST, LOW);  delay(100);
  digitalWrite(EPD_RST, HIGH); delay(100);

  command(0x00); data(0xF7); data(0x8A);                          // panel setting
  command(0x01); data(0x03); data(0x00); data(0x3F); data(0x3F); data(0x03);  // power setting
  command(0x03); data(0x00);                                      // power off sequence
  command(0x06); data(0x27); data(0x27); data(0x2F);              // booster soft start
  command(0x30); data(0x0D);                                      // PLL
  command(0x60); data(0x22);                                      // TCON
  command(0x82); data(0x07);                                      // VCOM DC
  command(0xE3); data(0x88);                                      // power saving
  command(0x41); data(0x00);                                      // temperature sensor
  command(0x61); data(0x80); data(0x00); data(0xFA);              // resolution 128 x 250
  command(0x65); data(0x00); data(0x00); data(0x00);              // gate/source start
  command(0x50); data(0xB7);                                      // VCOM/data interval

  // Old-image RAM starts undefined after reset; set it to white.
  command(0x10);
  for (size_t i = 0; i < BUFFER_BYTES; i++) data(0xFF);
}

void CrowPanelEPD::loadLut() {
  auto send = [this](uint8_t reg, const uint8_t* head) {
    command(reg);
    for (uint8_t i = 0; i < LUT_LEN; i++) data(i < LUT_HEAD_LEN ? head[i] : 0x00);
  };
  send(0x20, LUT_VCOM);
  send(0x21, LUT_WW);
  send(0x24, LUT_BB);
  // Elecrow's code swaps the 0x22 / 0x23 tables on alternate refreshes.
  if (!lut_toggle_) {
    send(0x22, LUT_R22);
    send(0x23, LUT_R23);
  } else {
    send(0x23, LUT_R22);
    send(0x22, LUT_R23);
  }
  lut_toggle_ = !lut_toggle_;
}

bool CrowPanelEPD::display(const uint8_t* buffer) {
  command(0x50); data(0xD7);
  command(0x13);                                   // new image
  for (size_t i = 0; i < BUFFER_BYTES; i++) data(~buffer[i]);  // panel: 1 = white
  loadLut();
  command(0x17); data(0xA5);                       // auto sequence: power on, refresh, power off
  return waitWhileBusy(30000);
}

void CrowPanelEPD::sleep() {
  command(0x07); data(0xA5);                       // deep sleep
  delay(20);
}
