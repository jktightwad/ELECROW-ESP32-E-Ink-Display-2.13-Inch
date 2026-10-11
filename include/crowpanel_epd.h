#pragma once
#include <Arduino.h>

// Minimal driver for the CrowPanel ESP32 E-Paper 2.13" (hardware matching Elecrow's
// "arduino-v1.2" example). The controller is driven in black/white mode with
// waveform tables loaded from the MCU; BUSY is LOW while the panel is working.
//
// Frame buffer layout: 250 rows x 16 bytes (128 pixels per row, 122 visible),
// most significant bit = leftmost pixel. This is the same layout as Adafruit
// GFX's GFXcanvas1(128, 250), so a canvas buffer can be passed in directly.
class CrowPanelEPD {
 public:
  static const uint16_t WIDTH = 128;    // controller columns (122 visible)
  static const uint16_t HEIGHT = 250;   // rows
  static const size_t BUFFER_BYTES = WIDTH / 8 * HEIGHT;
  // Only columns 6..127 of the 128 are on the glass (confirmed on hardware:
  // column 0 did not show). In GFXcanvas1 rotation 1 (landscape, 250 x 128) the
  // visible area is therefore y = 0..121.
  static const uint16_t FIRST_VISIBLE_COLUMN = 6;
  static const uint16_t VISIBLE_WIDTH = 122;

  // Powers the panel, resets it and loads its configuration.
  void begin();

  // Full refresh. Call begin() first (after power-up or sleep()). In `buffer`, a 1 bit is drawn BLACK and a 0 bit WHITE
  // (GFXcanvas1 convention: draw with colour 1 for black).
  // Returns false if the panel did not finish within the timeout.
  bool display(const uint8_t* buffer);

  // Fast refresh (Elecrow's "DU" waveform): only pixels that differ between
  // `previous` (what the screen shows now) and `buffer` change, with little
  // flashing. Repeated fast refreshes leave faint ghosting, so do a full
  // display() every so often. Same bit convention as display(). Call begin() first.
  bool displayFast(const uint8_t* buffer, const uint8_t* previous);

  // Puts the controller into deep sleep. Call begin() before the next display().
  void sleep();

 private:
  void writeByte(uint8_t b);
  void command(uint8_t c);
  void data(uint8_t d);
  void loadLut();
  void sendLut(uint8_t reg, const uint8_t* head, uint8_t head_len);
  bool waitWhileBusy(uint32_t timeout_ms);
};
